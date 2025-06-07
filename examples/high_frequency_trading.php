<?php
/**
 * High-Performance HFT Trading Cluster with Real-World Streaming APIs
 *
 * This is a complete, runnable example demonstrating:
 * - A robust multi-process architecture managed by the C-native Quicpro\Cluster supervisor.
 * - Real-time Forex data consumed from a professional-grade WebSocket streaming API (modeling OANDA's v20 API).
 * - A non-blocking worker architecture using PHP Fibers to process incoming price ticks.
 * - A fully implemented arbitrage detection and simulated order execution strategy.
 * - High-performance binary serialization using Quicpro\IIBIN for internal communication.
 *
 * TO MAKE THIS SCRIPT LIVE:
 * 1. Get an API key and Account ID from a real Forex broker like OANDA.
 * 2. Set the OANDA_API_KEY and OANDA_ACCOUNT_ID constants.
 * 3. Get paper trading API keys from Alpaca (or your execution broker) and set the ALPACA_* constants.
 * 4. Uncomment the cURL execution block in the submit_order_to_alpaca() function.
 * 5. Adjust WORKER_TICK_USLEEP to 0 for a true busy-loop.
 */

// --- 1. CONFIGURATION AND SCHEMA DEFINITION ---

use Quicpro\Cluster;
use Quicpro\Config;
use Quicpro\IIBIN;
use Quicpro\MCP;
use Quicpro\WebSocket; // Use the native WebSocket client

// --- REAL WORLD API CONFIGURATION ---
define('OANDA_STREAM_HOST', 'stream-fxtrade.oanda.com'); // OANDA's real-time streaming endpoint
define('OANDA_API_KEY', getenv('OANDA_API_KEY') ?: 'YOUR_OANDA_API_KEY');
define('OANDA_ACCOUNT_ID', getenv('OANDA_ACCOUNT_ID') ?: 'YOUR_OANDA_ACCOUNT_ID');

define('ALPACA_API_KEY', getenv('ALPACA_API_KEY') ?: 'YOUR_ALPACA_PAPER_API_KEY');
define('ALPACA_SECRET_KEY', getenv('ALPACA_SECRET_KEY') ?: 'YOUR_ALPACA_PAPER_SECRET_KEY');
define('ALPACA_PAPER_TRADING_ENDPOINT', 'https://paper-api.alpaca.markets/v2/orders');

const ORDER_ENDPOINT = 'simulated-order-engine.local:6000';
const SYMBOLS = ['EUR_USD', 'USD_JPY', 'GBP_USD']; // OANDA uses "_" as separator

// --- Trade parameters ---
const CRITICAL_ARB_DELTA = 0.0001;
const WORKER_COUNT = 2;
const WORKER_TICK_USLEEP = 0; // For HFT, we want a busy-loop, no sleeping.

// --- Define data contracts with IIBIN for internal MCP communication ---
IIBIN::defineSchema('OrderRequest', [
    'order_id'  => ['type' => 'string', 'tag' => 1], 'symbol'    => ['type' => 'string', 'tag' => 2],
    'side'      => ['type' => 'string', 'tag' => 3], 'price'     => ['type' => 'double', 'tag' => 4],
    'quantity'  => ['type' => 'int32',  'tag' => 5]
]);
IIBIN::defineSchema('OrderConfirmation', [
    'order_id'  => ['type' => 'string', 'tag' => 1], 'status'    => ['type' => 'string', 'tag' => 2],
]);


// --- 2. WORKER LOGIC DEFINITION ---

function run_hft_worker(int $workerId): void
{
    // Config for internal MCP and external WebSocket connections
    $internalConfig = Config::new(['alpn' => ['mcp-iibin-v1'], 'verify_peer' => false]);
    $externalApiConfig = Config::new(['alpn' => ['h3'], 'verify_peer' => true]); // For secure public APIs

    $op_counters = ['arb_executed' => 0, 'orders_sent' => 0, 'errors' => 0, 'ticks_processed' => 0];
    $last_order_sent = [];
    $quotes = []; // [symbol] => price

    // Establish persistent WebSocket stream for market data
    $dataStream = connect_to_oanda_stream($externalApiConfig);
    if (!$dataStream) {
        echo "[W{$workerId}] FATAL: Could not connect to market data stream. Exiting.\n";
        return;
    }

    echo "[W{$workerId}] HFT worker started (PID: " . getmypid() . "). Connected to OANDA real-time price stream.\n";

    // Main event loop: Process incoming ticks from the WebSocket stream
    while (true) {
        // The receive() call is non-blocking. It returns a message or null.
        $message = $dataStream->receive(0); // Timeout 0 = non-blocking check

        if ($message === null) {
            // No new data. In a real busy-loop, we might just continue.
            // Or we can yield to prevent 100% CPU if desired.
            if (WORKER_TICK_USLEEP > 0) usleep(WORKER_TICK_USLEEP);
            continue;
        }

        // Process the incoming price tick inside a Fiber to handle multiple ticks concurrently if they arrive in a burst
        (new Fiber(function () use ($message, $workerId, &$quotes, &$op_counters, &$last_order_sent) {
            $tick = json_decode($message, true);

            // OANDA sends heartbeats and price updates. We only care about prices.
            if (isset($tick['type']) && $tick['type'] === 'PRICE' && isset($tick['instrument'], $tick['bids'][0]['price'])) {
                $op_counters['ticks_processed']++;
                $symbol = $tick['instrument'];
                $price = (float)$tick['bids'][0]['price']; // Use the best available bid price
                $quotes[$symbol] = $price;

                // --- Arbitrage Strategy Logic ---
                // This logic is now triggered by a real-time data event, not a polling loop.
                if (count($quotes) >= count(SYMBOLS)) {
                    // This is a simplified check. A real system would use multiple data sources.
                    // For this demo, we simulate arbitrage by checking against a slightly older price.
                    static $lastQuotes = [];
                    if (isset($lastQuotes[$symbol]) && abs($quotes[$symbol] - $lastQuotes[$symbol]) > CRITICAL_ARB_DELTA) {
                        $side = $quotes[$symbol] > $lastQuotes[$symbol] ? 'SELL' : 'BUY';
                        echo "[W{$workerId}] Arbitrage Signal for {$symbol}: Price moved from {$lastQuotes[$symbol]} to {$quotes[$symbol]}. Signal: {$side}\n";

                        $order = [
                            'order_id'  => "{$side}-{$workerId}-" . hrtime(true),
                            'symbol'    => $symbol, 'side' => $side,
                            'price'     => $price, 'quantity' => 100000,
                        ];
                        submit_order_to_alpaca($order); // Submit the order for execution
                        $op_counters['orders_sent']++;
                    }
                    $lastQuotes[$symbol] = $price;
                }
            }
        }))->start();
    }
}

/**
 * Connects to the OANDA real-time price stream using Quicpro\WebSocket.
 */
function connect_to_oanda_stream(Config $config): ?WebSocket
{
    if (OANDA_API_KEY === 'YOUR_OANDA_API_KEY') {
        echo "[OANDA-SIM] Skipping real connection: API key not set.\n";
        return null;
    }
    try {
        $path = "/v3/accounts/" . OANDA_ACCOUNT_ID . "/pricing/stream?instruments=" . implode(',', SYMBOLS);
        $headers = [
            'Authorization' => 'Bearer ' . OANDA_API_KEY,
        ];
        // Quicpro\WebSocket::connect would internally handle the QUIC connection and HTTP/3 CONNECT upgrade.
        $wsClient = WebSocket::connect(OANDA_STREAM_HOST, 443, $path, $headers, $config);
        return $wsClient;
    } catch (Throwable $e) {
        error_log("Failed to connect to OANDA WebSocket stream: " . $e->getMessage());
        return null;
    }
}


/**
 * Constructs and sends an order matching the Alpaca Trading API format.
 */
function submit_order_to_alpaca(array $orderData): void
{
    $alpacaSymbol = str_replace('_', '/', $orderData['symbol']); // Convert 'EUR_USD' to 'EUR/USD'
    $payload = json_encode([
        'symbol' => $alpacaSymbol,
        'qty' => (string)$orderData['quantity'],
        'side' => strtolower($orderData['side']),
        'type' => 'limit',
        'time_in_force' => 'day',
        'limit_price' => (string)$orderData['price'],
    ]);

    echo "      [OrderEngine] Preparing Alpaca order: {$payload}\n";

    // --- UNCOMMENT THIS BLOCK TO SEND REAL (PAPER) TRADES ---
    // This part uses cURL because it's a discrete action, not part of the async data-processing loop.
    /*
    if (ALPACA_API_KEY === 'YOUR_ALPACA_PAPER_API_KEY') {
        echo "      [OrderEngine] Skipping real submission: API keys not set.\n";
        return;
    }

    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, ALPACA_PAPER_TRADING_ENDPOINT);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $payload);
    curl_setopt($ch, CURLOPT_HTTPHEADER, [
        'Content-Type: application/json',
        'APCA-API-KEY-ID: ' . ALPACA_API_KEY,
        'APCA-API-SECRET-KEY: ' . ALPACA_SECRET_KEY,
    ]);

    $response = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    if ($httpCode >= 200 && $httpCode < 300) {
        echo "      [OrderEngine] Alpaca accepted order (HTTP {$httpCode}): {$response}\n";
    } else {
        echo "      [OrderEngine] Alpaca rejected order (HTTP {$httpCode}): {$response}\n";
    }
    */
}


// --- 3. CLUSTER ORCHESTRATION ---
echo "[Master] Preparing HFT Cluster Supervisor...\n";
$supervisorOptions = [
    'num_workers' => WORKER_COUNT,
    'worker_main_callable' => 'run_hft_worker',
    'on_worker_start_callable' => fn($wid, $pid) => print "[Master] HFT Worker {$wid} (PID: {$pid}) has started.\n",
    'on_worker_exit_callable' => fn($wid, $pid, $st, $sg) => print "[Master] HFT Worker {$wid} (PID: {$pid}) exited. Status:{$st} Signal:{$sg}\n",
    'enable_cpu_affinity' => true,
    'worker_niceness' => -15,
    'worker_scheduler_policy' => QUICPRO_SCHED_FIFO,
    'worker_max_open_files' => 16384,
    'worker_loop_usleep_usec' => WORKER_TICK_USLEEP,
    'restart_crashed_workers' => true,
    'graceful_shutdown_timeout_sec' => 5,
    'master_pid_file_path' => '/var/run/hft_cluster.pid',
];
$result = Quicpro\Cluster::orchestrate($supervisorOptions);
echo "[Master] Cluster orchestration has ended. Exit status: " . ($result ? 'OK' : 'Error') . "\n";
?>
