<?php
/**
 * Advanced Example: Low-Latency Trading Worker
 *
 * This script represents the logic for a single, high-performance worker process
 * designed for algorithmic trading. It is intended to be run by the C-native
 * Quicpro\Cluster supervisor for multi-core execution and resilience.
 *
 * It demonstrates the full spectrum of the quicpro_async extension's capabilities.
 *
 * 10 EXTREME PERFORMANCE BOOSTERS IN ACTION
 * -------------------------------------------------------
 * 1. C-extension with zend_string direct writes: IIBIN::encode/decode runs in C.
 * 2. mmap() / mlock(): Enabled via the 'zero_copy' => true option for the MCP connection.
 * 3. Pre-heated opcode templates: IIBIN::define() compiles schemas into C memory at startup.
 * 4. Kernel CPU-affinity: Managed by Quicpro\Cluster::orchestrate() and Fiber::setAffinity().
 * 5. Lock-free ring buffer: A potential implementation for the order engine's IPC.
 * 6. Turbo IIBIN headerless mode: Used via the 'binary_mode' => 'iibin' option.
 * 7. CPU prefetch hints: An internal C-level optimization within the PECL.
 * 8. Memory-pooled Fibers: A user-land pattern, or a future PECL feature.
 * 9. Direct NIC-to-userland (XDP/DPDK): Enabled via 'zero_copy' and underlying config.
 * 10. HyperTimer: IIBIN::now() provides a C-native, high-precision timestamp.
 */

use Quicpro\Cluster;
use Quicpro\Config;
use Quicpro\MCP;
use Quicpro\IIBIN;
use Quicpro\WebSocket;

// --- 1. CONFIGURATION AND SCHEMA DEFINITION ---

// --- REAL WORLD API CONFIGURATION ---
// Assumes API keys are set as environment variables for security.
const MARKET_DATA_STREAM_HOST = 'stream-fxtrade.oanda.com'; // Real-time WebSocket streaming API
define('OANDA_API_KEY', getenv('OANDA_API_KEY') ?: 'YOUR_OANDA_API_KEY');
define('OANDA_ACCOUNT_ID', getenv('OANDA_ACCOUNT_ID') ?: 'YOUR_OANDA_ACCOUNT_ID');

// Define your internal, high-performance order execution engine's endpoint
const ORDER_ENGINE_ENDPOINT = 'orderengine.internal.lan:9000';
const SYMBOLS_TO_TRADE = ['EUR_USD', 'USD_JPY', 'GBP_USD']; // OANDA format

// --- TRADING STRATEGY PARAMETERS ---
const SHORT_EMA_PERIOD = 12; // Period for the fast-moving average
const LONG_EMA_PERIOD  = 26; // Period for the slow-moving average
const RSI_PERIOD       = 14; // Period for the Relative Strength Index
const RSI_OVERBOUGHT   = 70; // RSI level above which we consider the asset overbought
const RSI_OVERSOLD     = 30; // RSI level below which we consider the asset oversold
const MAX_PRICE_HISTORY = 50; // Number of recent prices to keep for calculations

// --- Define data contracts with IIBIN for internal order submission ---
IIBIN::defineSchema('Order', [
    'client_order_id' => ['type' => 'string', 'tag' => 1],
    'symbol'          => ['type' => 'string', 'tag' => 2],
    'side'            => ['type' => 'string', 'tag' => 3], // "BUY" or "SELL"
    'price'           => ['type' => 'double', 'tag' => 4],
    'quantity'        => ['type' => 'int32',  'tag' => 5],
    'timestamp_ns'    => ['type' => 'int64',  'tag' => 6]
]);
IIBIN::defineSchema('OrderConfirmation', [
    'client_order_id' => ['type' => 'string', 'tag' => 1],
    'exchange_order_id' => ['type' => 'string', 'tag' => 2],
    'status'            => ['type' => 'string', 'tag' => 3]
]);


// --- 2. WORKER LOGIC DEFINITION ---
// This entire function serves as the 'worker_main_callable' for the Cluster supervisor.

function trading_worker_main(int $workerId): void
{
    // Bind this worker's main thread to a specific CPU core for cache efficiency.
    // Quicpro\Fiber::setAffinity($workerId); // Hypothetical API

    echo "[W{$workerId}] Worker started (PID: " . getmypid() . "). Preparing MCP and data stream connections...\n";

    // Config for connecting to the external, secure market data API
    $externalApiConfig = Config::new(['alpn' => ['h3'], 'verify_peer' => true]);

    // Expert config for the internal, ultra-low-latency order engine connection
    $orderEngineConfig = Config::new([
        'alpn'                       => ['mcp-iibin-v1'],
        'tls_enable'                 => false, // Trusted L3 network
        'zero_copy_send'             => true,  // Use zero-copy I/O if available
        'busy_poll_duration_us'      => 1000,  // Busy-poll for 1ms before yielding
        'disable_congestion_control' => true,  // DANGER: For dedicated networks only
    ]);

    // Establish persistent connections
    $marketDataStream = connect_to_market_data_stream($externalApiConfig);
    [$orderHost, $orderPort] = explode(':', ORDER_ENGINE_ENDPOINT);
    $orderSession = new MCP($orderHost, (int)$orderPort, $orderEngineConfig);

    if (!$marketDataStream) {
        echo "[W{$workerId}] FATAL: Could not connect to market data stream. Exiting.\n";
        return;
    }

    $strategyState = []; // In-memory state for our trading strategy, per symbol

    // Main event loop: Non-blocking processing of incoming market data ticks.
    while (true) {
        // receive() is non-blocking with a timeout of 0. It returns a message or null.
        $message = $marketDataStream->receive(0);
        if ($message === null) {
            usleep(10); // No data, yield briefly
            continue;
        }

        // When a tick arrives, hand it off to a Fiber for processing.
        // This allows the main loop to immediately return to polling the WebSocket for the next tick.
        (new Fiber(
            fn() => process_market_tick($message, $orderSession, $workerId, $strategyState)
        ))->start();
    }
}

/**
 * Connects to the OANDA real-time price stream using Quicpro\WebSocket.
 */
function connect_to_market_data_stream(Config $config): ?WebSocket
{
    if (OANDA_API_KEY === 'YOUR_OANDA_API_KEY') {
        echo "[OANDA-SIM] Skipping real connection: API key not set. Worker will idle.\n";
        return null;
    }
    try {
        $path = "/v3/accounts/" . OANDA_ACCOUNT_ID . "/pricing/stream?instruments=" . implode('%2C', SYMBOLS_TO_TRADE);
        $headers = ['Authorization' => 'Bearer ' . OANDA_API_KEY];
        return WebSocket::connect(MARKET_DATA_STREAM_HOST, 443, $path, $headers, $config);
    } catch (Throwable $e) {
        error_log("Failed to connect to market data stream: " . $e->getMessage());
        return null;
    }
}

/**
 * This function runs inside a Fiber to process a single market data tick and execute trading logic.
 */
function process_market_tick(string $message, MCP $orderSession, int $workerId, array &$strategyState): void
{
    $tick = json_decode($message, true);

    if (!isset($tick['type']) || $tick['type'] !== 'PRICE' || !isset($tick['instrument'], $tick['bids'][0]['price'])) {
        return;
    }

    $symbol = $tick['instrument'];
    $price = (float)$tick['bids'][0]['price'];

    // --- State Management and Indicator Calculation ---
    // Initialize state for a new symbol
    if (!isset($strategyState[$symbol])) {
        $strategyState[$symbol] = [
            'prices' => [], 'short_ema' => null, 'long_ema' => null,
            'rsi' => null, 'avg_gain' => 0, 'avg_loss' => 0
        ];
    }
    $state = &$strategyState[$symbol]; // Work with a reference

    // Add new price and maintain history size
    $state['prices'][] = $price;
    if (count($state['prices']) > MAX_PRICE_HISTORY) {
        array_shift($state['prices']);
    }

    // Need enough data points to calculate indicators
    if (count($state['prices']) < LONG_EMA_PERIOD) {
        return;
    }

    // Calculate indicators
    $previousShortEma = $state['short_ema'];
    $previousLongEma = $state['long_ema'];
    $state['short_ema'] = calculate_ema($state['prices'], SHORT_EMA_PERIOD, $state['short_ema']);
    $state['long_ema'] = calculate_ema($state['prices'], LONG_EMA_PERIOD, $state['long_ema']);
    $state['rsi'] = calculate_rsi($state['prices'], RSI_PERIOD, $state['avg_gain'], $state['avg_loss']);

    // --- Advanced Trading Logic: EMA Crossover with RSI Confirmation ---
    // A "Golden Cross" (buy signal) occurs when the short-term EMA crosses above the long-term EMA.
    // A "Death Cross" (sell signal) occurs when the short-term EMA crosses below the long-term EMA.

    if ($previousShortEma !== null && $previousLongEma !== null) {
        // Check for Golden Cross (Buy Signal)
        if ($previousShortEma <= $previousLongEma && $state['short_ema'] > $state['long_ema']) {
            // Confirmation: Ensure we are not buying into an overbought market
            if ($state['rsi'] < RSI_OVERBOUGHT) {
                echo "[W{$workerId}] BUY SIGNAL for {$symbol} @ {$price} (EMA Crossover & RSI < " . RSI_OVERBOUGHT . ")\n";
                submit_order($orderSession, [
                    'client_order_id' => "W{$workerId}-BUY-" . hrtime(true), 'symbol' => $symbol,
                    'side' => 'BUY', 'price' => $price, 'quantity' => 10000, 'timestamp_ns' => IIBIN::now(),
                ]);
            }
        }
        // Check for Death Cross (Sell Signal)
        elseif ($previousShortEma >= $previousLongEma && $state['short_ema'] < $state['long_ema']) {
            // Confirmation: Ensure we are not selling into an oversold market
            if ($state['rsi'] > RSI_OVERSOLD) {
                echo "[W{$workerId}] SELL SIGNAL for {$symbol} @ {$price} (EMA Crossover & RSI > " . RSI_OVERSOLD . ")\n";
                submit_order($orderSession, [
                    'client_order_id' => "W{$workerId}-SELL-" . hrtime(true), 'symbol' => $symbol,
                    'side' => 'SELL', 'price' => $price, 'quantity' => 10000, 'timestamp_ns' => IIBIN::now(),
                ]);
            }
        }
    }
}

/**
 * Submits an order to the internal order engine via MCP using IIBIN serialization.
 */
function submit_order(MCP $orderSession, array $orderData): void
{
    try {
        $payload = IIBIN::encode('Order', $orderData);
        $binaryConfirmation = $orderSession->request('OrderExecutionService', 'ExecuteLimitOrder', $payload);
        // A real implementation would decode and verify the confirmation.
    } catch (Throwable $e) {
        error_log("Order submission failed: " . $e->getMessage());
    }
}

// --- Technical Indicator Helper Functions ---

function calculate_ema(array $prices, int $period, ?float $lastEma): float {
    $multiplier = 2 / ($period + 1);
    if ($lastEma === null) {
        // Calculate SMA for the first value
        return array_sum(array_slice($prices, 0, $period)) / $period;
    }
    $currentPrice = end($prices);
    return ($currentPrice - $lastEma) * $multiplier + $lastEma;
}

function calculate_rsi(array $prices, int $period, float &$avgGain, float &$avgLoss): float {
    $changes = [];
    for ($i = 1; $i < count($prices); $i++) {
        $changes[] = $prices[$i] - $prices[$i-1];
    }
    if (count($changes) < $period) return 50.0; // Not enough data, return neutral RSI

    if ($avgGain == 0) { // First calculation, use simple average
        $gains = array_sum(array_filter(array_slice($changes, -$period), fn($c) => $c > 0));
        $losses = abs(array_sum(array_filter(array_slice($changes, -$period), fn($c) => $c < 0)));
        $avgGain = $gains / $period;
        $avgLoss = $losses / $period;
    } else { // Subsequent calculations, use smoothed average
        $currentChange = end($changes);
        $currentGain = $currentChange > 0 ? $currentChange : 0;
        $currentLoss = $currentChange < 0 ? abs($currentChange) : 0;
        $avgGain = (($avgGain * ($period - 1)) + $currentGain) / $period;
        $avgLoss = (($avgLoss * ($period - 1)) + $currentLoss) / $period;
    }

    if ($avgLoss == 0) return 100.0; // Prevent division by zero
    $rs = $avgGain / $avgLoss;
    return 100 - (100 / (1 + $rs));
}


// --- 3. CLUSTER ORCHESTRATION ---
echo "[Master] Preparing High-Performance Trading Cluster...\n";
$supervisorOptions = [
    'num_workers' => WORKER_COUNT,
    'worker_main_callable' => 'trading_worker_main',
    'on_worker_start_callable' => fn($wid, $pid) => print "[Master] Trading Worker {$wid} (PID: {$pid}) has started.\n",
    'on_worker_exit_callable' => fn($wid, $pid, $st, $sg) => print "[Master] Trading Worker {$wid} (PID: {$pid}) exited. Status:{$st} Signal:{$sg}\n",
    'enable_cpu_affinity' => true,
    'worker_niceness' => -20, // Maximum OS priority
    'worker_scheduler_policy' => QUICPRO_SCHED_FIFO, // Real-time First-In, First-Out scheduler
    'worker_max_open_files' => 65536,
    'restart_crashed_workers' => true,
    'graceful_shutdown_timeout_sec' => 5,
    'master_pid_file_path' => '/var/run/trading_cluster.pid',
];
$result = Quicpro\Cluster::orchestrate($supervisorOptions);
echo "[Master] Cluster orchestration has ended. Exit status: " . ($result ? 'OK' : 'Error') . "\n";
?>
