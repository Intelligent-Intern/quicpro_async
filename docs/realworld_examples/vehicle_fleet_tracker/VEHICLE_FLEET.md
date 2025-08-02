# Use Case: Resilient IoT Fleet Tracking with Connection Migration

This document provides a detailed, expert-level walkthrough for using the `quicpro_async` framework to build a highly resilient IoT application: a real-time vehicle fleet tracker that reads data directly from hardware.

## 1. The Business Problem & Technical Challenge

A modern logistics company needs to track its fleet of hundreds of delivery vehicles in real-time. The goal is not just to see GPS coordinates on a map, but to stream a rich set of telemetry data - location, speed, fuel consumption, engine RPM, tire pressure - directly from the vehicle's CAN bus to a central analytics platform. This data is critical for route optimization, predictive maintenance, and driver safety.

The primary technical challenge is the highly dynamic and often unreliable network environment of a moving vehicle. A connection must survive:

* **Network Handoffs:** Seamlessly switching between different cellular towers and technologies (e.g., 4G to 5G).
* **Interface Changes:** Automatically switching from a cellular network to a depot's Wi-Fi network upon arrival, and back again upon departure.
* **Temporary Outages:** Maintaining the session through short periods of total signal loss, such as in tunnels or remote areas.

A traditional TCP-based solution fails here. Because a TCP connection is rigidly defined by a 4-tuple (source IP, source port, destination IP, destination port), any change to the client's IP address instantly invalidates and breaks the connection. This results in data loss and requires a full, slow reconnection cycle, including a new TLS handshake.

## 2. The `quicpro_async` Solution: Transparent Connection Migration

QUIC, the transport protocol underlying `quicpro_async`, was explicitly designed to solve this problem. It decouples the logical connection from the underlying network path.

The key is the **Connection ID**, a unique identifier for the session. The client and server use this ID to recognize packets belonging to a connection, regardless of the source or destination IP address.

When a vehicle's network interface changes (e.g., from cellular to Wi-Fi), the `quicpro_async` client simply begins sending its QUIC packets from the new IP address, but with the **same Connection ID**. The server, recognizing the ID, initiates a quick "path validation" process to confirm the client's ownership of the new address and then seamlessly continues the existing session.

For the PHP application developer, this entire process is **completely transparent**. The `Session::poll()` method handles all migration logic internally. There is no need to write complex reconnection or state-synchronization code; the data stream simply continues as if nothing happened.

### Visualizing the Flow

This diagram shows how the QUIC session, identified by `ID: ABC`, persists as the vehicle's network path changes from Cellular to Wi-Fi.

~~~mermaid
graph LR
    subgraph "Network Path 1"
        Client(fa:fa-truck Vehicle Client) -- QUIC Connection ID: ABC --> Cellular[fa:fa-tower Cellular Network];
    end

    subgraph "Network Path 2"
        Client -- QUIC Connection ID: ABC --> WiFi[fa:fa-wifi Depot Wi-Fi];
    end
    
    Cellular --> Server(fa:fa-server Central Server);
    WiFi --> Server;

    style Client fill:#00796b,color:white
    style Server fill:#512da8,color:white
~~~

## 3. Expert Configuration Deep Dive

The `Quicpro\Config` object is tuned to maximize resilience for this mobile IoT use case.

### `disable_active_migration: false`
* **Type**: `boolean`
* **Default**: `false`
* **Justification**: This is the default setting, but we state it explicitly to emphasize its importance. Keeping active migration enabled allows the client to proactively probe new potential network paths and switch to them without waiting for the old path to time out, further reducing interruption time.

### `alpn: ['geostream/1.0']`
* **Type**: `array`
* **Default**: `['h3']`
* **Justification**: While we could use HTTP/3, streaming raw telemetry doesn't require the overhead of HTTP headers, methods, and status codes. By defining a custom, lean Application-Layer Protocol (`geostream/1.0`), we reduce the size of every data packet, saving bandwidth and battery life on the IoT device. `quicpro_async` allows sending raw data over QUIC streams when a non-HTTP/3 ALPN is negotiated.

### `max_idle_timeout_ms: 180000`
* **Type**: `integer`
* **Default**: `10000` (10 seconds)
* **Justification**: A long, 3-minute idle timeout is crucial. It gives the vehicle enough time to pass through a long tunnel or an area with no signal without the server prematurely closing the connection. As soon as the vehicle regains connectivity, it can resume sending data over the existing, still-valid session.

## 4. Implementation: The Real-Time PHP Tracker Client

The following script is a complete, runnable example of the client logic that would run inside a vehicle. It opens the GPS device, establishes a QUIC connection, and enters a non-blocking loop to process and stream data as it arrives from the hardware.

~~~php
<?php
declare(strict_types=1);

/**
 * This script requires the Quicpro PECL extension to be enabled in php.ini.
 */
if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\Session;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

// --- Application Constants ---
const GPS_DEVICE_PATH = '/dev/ttyACM0';
const VEHICLE_ID = 'TRUCK-042';

/**
 * Parses a raw NMEA 0183 GGA sentence to extract latitude and longitude.
 * Returns null if the sentence is not a valid GGA sentence.
 * In a real application, a more comprehensive NMEA parsing library could be used.
 */
function parse_nmea_gga_sentence(string $sentence): ?array
{
    $parts = explode(',', trim($sentence));
    if (count($parts) < 7 || $parts[0] !== '$GPGGA' || $parts[6] === '0') {
        return null; // No valid fix
    }

    $lat_raw = (float)$parts[2];
    $lat_deg = floor($lat_raw / 100);
    $lat_min = $lat_raw - ($lat_deg * 100);
    $lat = $lat_deg + ($lat_min / 60);
    if ($parts[3] === 'S') {
        $lat = -$lat;
    }

    $lon_raw = (float)$parts[4];
    $lon_deg = floor($lon_raw / 100);
    $lon_min = $lon_raw - ($lon_deg * 100);
    $lon = $lon_deg + ($lon_min / 60);
    if ($parts[5] === 'W') {
        $lon = -$lon;
    }

    return ['lat' => $lat, 'lon' => $lon];
}

// --- Main Application ---

try {
    // 1. Define the data contract for our telemetry messages using IIBIN.
    // This ensures payloads are small, fast, and type-safe.
    IIBIN::defineSchema('GeoTelemetry', [
        'vehicle_id' => ['type' => 'string', 'tag' => 1],
        'lat'        => ['type' => 'double', 'tag' => 2],
        'lon'        => ['type' => 'double', 'tag' => 3],
        'speed_kmh'  => ['type' => 'uint32', 'tag' => 4],
        'timestamp'  => ['type' => 'uint64', 'tag' => 5],
    ]);

    // 2. Create the specialized configuration for a mobile client.
    $trackerConfig = Config::new([
        'verify_peer'              => true,
        'disable_active_migration' => false, // Ensure Connection Migration is active.
        'alpn'                     => ['geostream/1.0'],
        'max_idle_timeout_ms'      => 180000,
    ]);
    
    // 3. Open the GPS hardware device as a non-blocking stream.
    if (!file_exists(GPS_DEVICE_PATH)) {
        throw new \RuntimeException("GPS device not found at " . GPS_DEVICE_PATH);
    }
    $gpsStream = fopen(GPS_DEVICE_PATH, 'r');
    if (!$gpsStream || !stream_set_blocking($gpsStream, false)) {
        throw new \RuntimeException("Failed to open GPS device in non-blocking mode.");
    }
    
    // 4. Establish the long-lived QUIC session to the server.
    $session = new Session('fleet-server.my-company.com', 443, $trackerConfig);
    echo "Tracker online. Listening to GPS and streaming to server...\n";

    // 5. Start the main, non-blocking event loop.
    while ($session->isConnected()) {
        // First, always drive the network connection state machine. This call
        // handles sending/receiving packets and transparently manages migration.
        // The timeout is non-blocking.
        $session->poll(10); // Poll every 10ms.

        // Second, attempt to read a new line from the GPS hardware.
        // Since the stream is non-blocking, this returns immediately if no data is available.
        $nmeaSentence = fgets($gpsStream);

        if ($nmeaSentence !== false && !empty($nmeaSentence)) {
            // A new line was received from the GPS module.
            $coords = parse_nmea_gga_sentence($nmeaSentence);

            if ($coords !== null) {
                // We successfully parsed the coordinates. Send them to the server.
                $payload = IIBIN::encode('GeoTelemetry', [
                    'vehicle_id' => VEHICLE_ID,
                    'lat'        => $coords['lat'],
                    'lon'        => $coords['lon'],
                    'speed_kmh'  => 85, // This would also come from another NMEA sentence (e.g., $GPVTG).
                    'timestamp'  => time(),
                ]);
                
                // Send the binary payload on a unidirectional stream, which is highly
                // efficient as we do not need a direct response for each packet.
                $session->sendUnidirectionalStreamData($payload);
                echo "Sent GPS Fix: Lat: " . round($coords['lat'], 5) . ", Lon: " . round($coords['lon'], 5) . "\n";
            }
        }
    }

} catch (QuicproException | \RuntimeException $e) {
    echo "A fatal error occurred: " . $e->getMessage() . "\n";
    if (isset($gpsStream) && is_resource($gpsStream)) {
        fclose($gpsStream);
    }
}
~~~

## Summary

By leveraging `quicpro_async`, QUIC's native Connection Migration feature, and an efficient binary serialization format like `IIBIN`, developing highly robust and performant mobile IoT applications in PHP is drastically simplified and optimized. The framework handles the complex, low-level network challenges, allowing developers to focus purely on the application logic.