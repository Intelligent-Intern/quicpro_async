<?php
/**
 * Quicpro\Config: Beginner-Friendly Example
 *
 * This script demonstrates how to create a simple, secure, and effective configuration
 * for a QUIC client using the quicpro_async extension.
 *
 * It follows modern PHP best practices by using `use` statements for brevity.
 *
 * ---
 *
 * Prerequisites for this script to run:
 *
 * 1. The `quicpro_async.so` extension must be enabled in your php.ini:
 * `extension=quicpro_async.so`
 *
 * 2. The configuration override policy must be enabled in php.ini,
 * because this example passes an options array to `Config::new()`:
 * `quicpro.allow_config_override = 1`
 *
 */
declare(strict_types=1);

// Import the necessary classes into the current namespace for clean, readable code.
use Quicpro\Config;
use Quicpro\Session;
use Quicpro\Exception\PolicyViolationException;
use Quicpro\Exception\QuicproException;

// Ensure the extension is loaded.
if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded. Please check your php.ini.\n");
}

try {
    echo "1. Creating a secure configuration...\n";

    // Create a new configuration object.
    $config = Config::new([
        'verify_peer' => true, // Essential for security when connecting to public services.
        'alpn'        => ['h3'], // Required to negotiate HTTP/3.
    ]);

    echo "2. Establishing a QUIC session to www.google.com...\n";

    // The config object is passed to the session constructor.
    $session = new Session('www.google.com', 443, $config);

    // The isConnected() method would handle the necessary internal polling
    // to wait for the handshake to complete or fail.
    if ($session->isConnected()) {
        echo "3. Connection successful! Handshake complete.\n";
    } else {
        // This path might not be reached if construction/handshake throws.
        echo "3. Connection failed during handshake.\n";
    }

    echo "4. Closing the session.\n";
    $session->close();

    echo "\n✅ Beginner example completed successfully.\n";

} catch (PolicyViolationException $e) {
    // A specific catch block for the override policy. This provides a clear error message.
    echo "\n❌ CONFIGURATION POLICY ERROR:\n";
    echo "   Message: " . $e->getMessage() . "\n";
    echo "   Please ensure 'quicpro.allow_config_override = 1' is set in your php.ini to run this example.\n";
    exit(1);

} catch (QuicproException $e) {
    // A general catch block for all other extension-related errors.
    echo "\n❌ AN ERROR OCCURRED:\n";
    echo "   Class:   " . get_class($e) . "\n";
    echo "   Message: " . $e->getMessage() . "\n";
    exit(1);
}