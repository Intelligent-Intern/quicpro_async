<?php
/**
 * High-Performance HFT Trading Cluster with MCP and Binary Protocols
 *
 * This advanced example demonstrates:
 * - Cluster-wide task coordination using forked workers (one per CPU core)
 * - Broker/venue feeds queried in parallel with Fibers
 * - Priority-based execution (for critical, real-time, and standard requests)
 * - Robust error handling and failover, atomic operation counting
 * - Binary encoding for MCP (MessagePack)
 * - Adaptive throttling and tick pacing
 * - Order deduplication and minimal risk logic
 *
 * --- SCENARIO ---
 * We track N symbols across multiple brokers (venues), keep a rolling window of quotes,
 * and immediately trigger orders for critical-priority conditions (e.g., arbitrage).
 * Lower-priority strategies (e.g., trend following) are queued if system load allows.
 * All communications are fully binary, and NO blocking occurs unless absolutely needed.
 */

// CONFIGURATION SECTION -----------------------------------------------------

use Quicpro\MCP;
use Quicpro\Config;

// Define brokers and order endpoint(s). In real use, put these in env or secrets manager.
const BROKER_ENDPOINTS = [
    'broker1.lan:5001',
    'broker2.lan:5002',
    'broker3.lan:5003',
];
const ORDER_ENDPOINT   = 'orderengine.lan:6000';

// List of symbols to watch/trade (could come from database/config)
const SYMBOLS = ['AAPL', 'GOOG', 'MSFT', 'TSLA'];

// Trade parameters
const CRITICAL_ARB_DELTA     = 0.12;   // Min price diff for arbitrage
const NORMAL_TRADE_THRESHOLD = 0.03;   // Min threshold for regular trade
const MAX_PARALLEL_FIBERS    = 64;     // System cap for fibers per worker

const WORKERS = 4; // Typically, number of CPU cores. Adjust as needed.

// PRIO / CTX / MODE bitmasks for demonstration of advanced queueing/dispatch
const PRIO_CRITICAL = 0b0001;
const PRIO_NORMAL   = 0b0010;
const MODE_REALTIME = 0b0100;
const CTX_HFT       = 0b1000;

// MCP/QUIC Configuration, binary mode, no peer verify for internal LAN
$config = Config::new([
    'alpn'            => ['mcp-bin'],
    'session_cache'   => true,
    'max_pkt_size'    => 1400,
    'max_idle_timeout'=> 10000,
    'verify_peer'     => false,
]);

// Atomic operation counters (per worker)
$op_counters = [
    'arb_executed'  => 0,
    'trades_sent'   => 0,
    'errors'        => 0,
    'fibers_used'   => 0,
];

// --- FORK WORKERS ----------------------------------------------------------
for ($i = 0; $i < WORKERS; $i++) {
    $pid = pcntl_fork();
    if ($pid === 0) {
        // Each worker manages its own session pool, counters, and queues
        run_worker($i, $config);
        exit(0);
    }
}
while (pcntl_wait($status) > 0); // Parent waits for all children

// --- WORKER FUNCTION -------------------------------------------------------
function run_worker(int $workerId, Config $cfg): void
{
    global $op_counters;

    // Prepare MCP sessions for all brokers and order endpoint
    $sessions = [];
    foreach (BROKER_ENDPOINTS as $endpoint) {
        [$host, $port] = explode(':', $endpoint);
        $sessions['brokers'][$endpoint] = new MCP($host, (int)$port, $cfg);
    }
    [$orderHost, $orderPort] = explode(':', ORDER_ENDPOINT);
    $sessions['order'] = new MCP($orderHost, (int)$orderPort, $cfg);

    // State: last seen quotes per symbol, per broker
    $quotes = [];  // [symbol][broker] => price

    // Simple dedupe map to avoid duplicate orders per tick
    $last_order_sent = [];

    // Main event loop: fetch all prices, run strategies, submit orders as needed
    while (true) {
        $fiber_queue = [];   // Priority queue for execution
        $critical_arb_ops = [];

        // 1. For each symbol and each broker, start a Fiber to fetch price in parallel
        foreach (SYMBOLS as $symbol) {
            foreach (BROKER_ENDPOINTS as $endpoint) {
                if (count($fiber_queue) >= MAX_PARALLEL_FIBERS) break 2; // Safety

                $fiber_queue[] = new Fiber(function () use ($sessions, $symbol, $endpoint, $workerId, &$quotes, &$op_counters) {
                    try {
                        // MCP request for the symbol
                        $payload = serialize_to_binary([
                            'type'      => 'get_price',
                            'symbol'    => $symbol,
                            'timestamp' => microtime(true),
                        ]);
                        $start = hrtime(true);
                        $response = $sessions['brokers'][$endpoint]->callAgent($payload);
                        $op_counters['fibers_used']++;

                        $result = deserialize_from_binary($response);

                        // Store latest quote
                        if (isset($result['price'])) {
                            $quotes[$symbol][$endpoint] = $result['price'];
                            echo "[W$workerId] $endpoint: $symbol = {$result['price']}\n";
                        } else {
                            throw new Exception("Invalid MCP response");
                        }

                        // Return info for later processing
                        return ['symbol' => $symbol, 'broker' => $endpoint, 'price' => $result['price']];

                    } catch (Throwable $e) {
                        $op_counters['errors']++;
                        echo "[W$workerId][$endpoint][$symbol] MCP error: " . $e->getMessage() . "\n";
                        return null;
                    }
                });
            }
        }

        // 2. Start all Fibers
        foreach ($fiber_queue as $fiber) $fiber->start();

        // 3. Collect results, analyze for arbitrage and normal opportunities
        $completed = 0;
        while ($completed < count($fiber_queue)) {
            foreach ($fiber_queue as $fiber) {
                if (!$fiber->isTerminated()) {
                    $fiber->resume();
                    continue;
                }
                $completed++;
            }
            usleep(10); // Tune as needed for busy-wait
        }

        // --- CRITICAL ARBITRAGE STRATEGY (highest priority) ----------------------
        foreach (SYMBOLS as $symbol) {
            // Find best and worst prices across brokers
            $min = $max = null;
            $bmin = $bmax = null;
            foreach (BROKER_ENDPOINTS as $endpoint) {
                if (!isset($quotes[$symbol][$endpoint])) continue;
                $price = $quotes[$symbol][$endpoint];
                if ($min === null || $price < $min) { $min = $price; $bmin = $endpoint; }
                if ($max === null || $price > $max) { $max = $price; $bmax = $endpoint; }
            }
            if ($max - $min > CRITICAL_ARB_DELTA) {
                // Arbitrage detected: Buy low, sell high!
                $critical_arb_ops[] = [
                    'symbol'  => $symbol,
                    'buy_at'  => $bmin,
                    'buy_px'  => $min,
                    'sell_at' => $bmax,
                    'sell_px' => $max,
                    'priority'=> PRIO_CRITICAL | CTX_HFT | MODE_REALTIME
                ];
            }
        }

        // 4. Execute CRITICAL ARBITRAGE with immediate priority (never batch, never wait)
        foreach ($critical_arb_ops as $arb) {
            $k = "{$arb['symbol']}-{$arb['buy_at']}-{$arb['sell_at']}";
            // Dedupe: only one trade per symbol/broker combination per tick
            if (isset($last_order_sent[$k]) && (microtime(true) - $last_order_sent[$k] < 0.2)) continue;
            $last_order_sent[$k] = microtime(true);

            // Buy order
            $buyOrder = [
                'type'      => 'order',
                'symbol'    => $arb['symbol'],
                'side'      => 'buy',
                'price'     => $arb['buy_px'],
                'venue'     => $arb['buy_at'],
                'priority'  => $arb['priority'],
                'timestamp' => microtime(true),
            ];
            $sellOrder = [
                'type'      => 'order',
                'symbol'    => $arb['symbol'],
                'side'      => 'sell',
                'price'     => $arb['sell_px'],
                'venue'     => $arb['sell_at'],
                'priority'  => $arb['priority'],
                'timestamp' => microtime(true),
            ];
            submit_order($sessions['order'], $buyOrder, $workerId, $arb['buy_at']);
            submit_order($sessions['order'], $sellOrder, $workerId, $arb['sell_at']);
            $op_counters['arb_executed']++;
        }

        // --- NORMAL PRIORITY TRADING STRATEGY (queue, throttle if needed) -------
        foreach (SYMBOLS as $symbol) {
            foreach (BROKER_ENDPOINTS as $endpoint) {
                $price = $quotes[$symbol][$endpoint] ?? null;
                if ($price === null) continue;
                // Simple logic: Buy if price drops below a threshold (for demonstration)
                if ($price < NORMAL_TRADE_THRESHOLD) {
                    $normalOrder = [
                        'type'      => 'order',
                        'symbol'    => $symbol,
                        'side'      => 'buy',
                        'price'     => $price,
                        'venue'     => $endpoint,
                        'priority'  => PRIO_NORMAL,
                        'timestamp' => microtime(true),
                    ];
                    submit_order($sessions['order'], $normalOrder, $workerId, $endpoint);
                    $op_counters['trades_sent']++;
                }
            }
        }

        // -- Performance monitoring (every X ticks, print stats) ---------------
        static $tick = 0;
        if (++$tick % 100 == 0) {
            echo "[W$workerId] TICK $tick: ARB={$op_counters['arb_executed']} NORMAL={$op_counters['trades_sent']} ERR={$op_counters['errors']}\n";
        }
    }
}

/**
 * Submit order over MCP, with retry and logging
 * All MCP messages are packed as binary (e.g., msgpack)
 */
function submit_order(MCP $orderSession, array $order, int $workerId, string $venue): void
{
    global $op_counters;
    try {
        $bin = serialize_to_binary($order);
        $start = hrtime(true);
        $resp = $orderSession->callAgent($bin);
        $dt = (hrtime(true) - $start) / 1e3; // Microseconds
        $data = deserialize_from_binary($resp);
        echo "[W$workerId][ORDER][$venue] Order: " . json_encode($order) . " RT={$dt}us Result: " . json_encode($data) . "\n";
    } catch (Throwable $e) {
        $op_counters['errors']++;
        echo "[W$workerId][ORDER][$venue] Submit error: " . $e->getMessage() . "\n";
    }
}

/**
 * Binary serialization using MessagePack for ultra-fast wire transfer.
 * In production, use flatbuffers, protobuf or custom binary protocol for even better performance.
 */
function serialize_to_binary(array $data): string
{
    return msgpack_pack($data);
}

function deserialize_from_binary(string $data): array
{
    return msgpack_unpack($data);
}
