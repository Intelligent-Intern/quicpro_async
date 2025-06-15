# Configuring the Native CORS Handler

The `quicpro_async` extension includes a high-performance, C-level handler for Cross-Origin Resource Sharing (CORS) to simplify development and maximize performance. This document explains the two ways to configure this feature: globally for the entire server via `php.ini`, or on a per-session basis using the `Quicpro\Config` object.

This built-in handler is designed to cover the most common use cases. For highly complex or dynamic CORS policies (e.g., based on database lookups), the native handler can be disabled, allowing for a full implementation in PHP userland.

The ability to override the global `php.ini` setting is controlled by the `quicpro.allow_config_override` directive.

## Method 1: Global Configuration via `php.ini`

This method is ideal for system administrators who want to enforce a baseline CORS policy for all applications on a server. The policy is defined directly in the `php.ini` file.

To implement this, you would add the following directive to your `php.ini`:

~~~ini
; php.ini
;
; Sets a global CORS policy for all quicpro_async applications. This value
; is a comma-separated string of allowed origin domains.
; Use '*' for a public wildcard or specify one or more domains.
quicpro.cors_allowed_origins = "https://my-app.com,https://staging.my-app.com"
        
; This master switch determines if the setting above can be overridden by PHP code.
quicpro.allow_config_override = 1
~~~

When this `php.ini` directive is set, any `quicpro_async` server started without a specific CORS policy in its `Config` object will automatically inherit and enforce this global policy.

**PHP Code Example:**

The following PHP code does **not** specify any CORS rules itself. It will automatically use the policy defined in `php.ini` because the `cors_allowed_origins` key is absent from its `Config` object.

~~~php
<?php
// This server will automatically use the CORS policy from php.ini
// because no `cors_allowed_origins` key is provided here.

$configUsingGlobalPolicy = Quicpro\Config::new([
    'cert_file' => './certs/server.pem',
    'key_file'  => './certs/server.key',
]);
        
$server = new Quicpro\Server('0.0.0.0', 4433, $configUsingGlobalPolicy);
// $server->run();
~~~

## Method 2: Per-Session Configuration and Userland Logic

This method provides developers with fine-grained control. By explicitly disabling the C-level handler, you can implement complex, dynamic CORS policies directly in your PHP application. This is necessary when, for example, the list of allowed origins is stored in a database.

The following is a complete, runnable WebSocket echo server that demonstrates this pattern.

**PHP Code Example (`dynamic_cors_websocket_server.php`):**

~~~php
<?php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\Server;
use Quicpro\Session;
use Quicpro\Exception\QuicproException;

/**
 * A dummy function to simulate checking an origin against a database.
 * In a real application, this would query your database or a config service.
 */
function is_origin_allowed_dynamically(string $origin): bool
{
    $allowedFromDB = [
        'https://my-production-app.com',
        'https://user-a.dev.my-app.com',
        'http://localhost:8080'
    ];
    return in_array($origin, $allowedFromDB, true);
}

/**
 * The handler logic for a single connection. It performs the CORS check
 * before upgrading to a WebSocket and starting the echo service.
 * This runs inside a Fiber for each connection.
 */
function handle_connection(Session $session): void
{
    try {
        $request = $session->receiveRequest();
        if (!$request) { return; }

        $origin = $request->getHeader('Origin');
        
        // 1. Enforce the CORS Policy
        if (!$origin || !is_origin_allowed_dynamically($origin)) {
            $session->sendHttpResponse(403, [], "Forbidden Origin");
            return;
        }
        
        // 2. Handle the OPTIONS Preflight Request
        if ($request->getMethod() === 'OPTIONS') {
            $session->sendHttpResponse(204, [
                'Access-Control-Allow-Origin' => $origin,
                'Access-Control-Allow-Methods' => 'GET, OPTIONS',
                'Access-Control-Allow-Headers' => 'Authorization',
                'Access-Control-Max-Age' => '86400',
            ]);
            return;
        }

        // 3. Add CORS header to the actual GET request for the WebSocket upgrade
        $session->addResponseHeader('Access-Control-Allow-Origin', $origin);
        
        // 4. Upgrade the connection to a WebSocket
        $websocketStream = $session->upgrade('/echo');
        if (!$websocketStream) { return; }

        // 5. Run the server's main function (in this case, an echo service)
        echo "Client from origin '{$origin}' connected. Echoing messages...\n";
        while (is_resource($websocketStream) && !feof($websocketStream)) {
            $data = fread($websocketStream, 8192);
            if (!empty($data)) {
                fwrite($websocketStream, "ECHO: " . $data);
            }
            // The poll keeps the underlying QUIC connection alive.
            $session->poll(50);
        }
    } catch (QuicproException $e) {
        echo "Connection error: " . $e->getMessage() . "\n";
    } finally {
        if ($session->isConnected()) {
            $session->close();
        }
    }
}

// --- Main Server Setup ---
try {
    $dynamicCorsConfig = Config::new([
        'cert_file' => './certs/server.pem',
        'key_file'  => './certs/server.key',
        // This explicitly disables the C-level handler, forcing a fallback
        // to our PHP userland logic in `handle_connection`.
        'cors_allowed_origins' => false,
    ]);

    $server = new Server('0.0.0.0', 4435, $dynamicCorsConfig);
    echo "Dynamic CORS WebSocket Echo Server listening on udp://0.0.0.0:4435...\n";
    
    while ($session = $server->accept()) {
        (new Fiber('handle_connection'))->start($session);
    }

} catch (QuicproException | \Exception $e) {
    die("A fatal server error occurred: " . $e->getMessage() . "\n");
}
~~~

Or use the build-in websocket capabilities with php.ini declared CORS 

~~~php

<?php

/**
 * quicpro_async: Demonstrating Native, C-Level CORS Handling
 *
 * This script demonstrates the simplest and most performant way to handle CORS
 * with the `quicpro_async` framework. The entire CORS policy is defined globally
 * in `php.ini`, and the C-extension handles all preflight requests and header
 * management automatically.
 *
 * As a result, the PHP application code is completely clean of any CORS-related
 * logic, allowing the developer to focus solely on the application's features.
*/

declare(strict_types=1);

// This script requires the Quicpro PECL extension.
if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\WebSocket\Server as WebSocketServer;
use Quicpro\WebSocket\Connection;
use Quicpro\Exception\QuicproException;


/**
 * =========================================================================
 * REQUIRED php.ini CONFIGURATION FOR THIS EXAMPLE TO WORK
 * =========================================================================
 *
 * To enable the native C-level CORS handler, you must configure it in your
 * php.ini file.
 *
  
  ; php.ini
 
  ; Define the global CORS policy. This allows any origin.
  ; For production, you should restrict this to your specific frontend domain(s),
  ; e.g., "https://my-app.com,https://staging.my-app.com"
  quicpro.cors_allowed_origins = "*"
 
  ; Optional but recommended for production: prevent PHP code from
  ; overriding this global security policy.
  quicpro.allow_config_override = 0
 
 */


// --- Main Server Setup ---

try {
    // 1. Create the server configuration.
    // Notice that this Config object contains NO 'cors_allowed_origins' key.
    // The server will therefore automatically inherit and apply the global
    // policy defined in php.ini.
    $serverConfig = Config::new([
        'cert_file' => './certs/server.pem',
        'key_file'  => './certs/server.key',
        'alpn'      => ['h3'],
    ]);

    // 2. Instantiate the high-level WebSocket server.
    $wsServer = new WebSocketServer('0.0.0.0', 4433, $serverConfig);

    /**
     * 3. Register Event Handlers.
     * The application logic is now pure and simple, with no CORS checks.
     */

    // The 'connect' handler is now trivial. The C-core has already validated
    // the Origin and handled any OPTIONS preflight request before this
    // PHP code is even executed.
    $wsServer->on('connect', function (Connection $connection) {
        echo "Client #{$connection->getId()} connected successfully. CORS check passed natively.\n";
        $connection->send("Welcome to the C-handled CORS Echo Server!");
    });

    // The 'message' handler is unchanged, focusing only on business logic.
    $wsServer->on('message', function (Connection $connection, string $message) {
        echo "Received '{$message}' from client #{$connection->getId()}. Echoing back...\n";
        $connection->send("ECHO: " . $message);
    });

    // The 'close' event handler is also unchanged.
    $wsServer->on('close', function (Connection $connection) {
        echo "Client #{$connection->getId()} disconnected.\n";
    });

    echo "Native CORS WebSocket Echo Server listening on udp://0.0.0.0:4433...\n";
    
    // 4. Start the server's event loop.
    $wsServer->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal server error occurred: " . $e->getMessage() . "\n");
}
~~~