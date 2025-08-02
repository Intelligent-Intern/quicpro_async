### **Einstiegspapier für Entwickler: Die `quicpro_async` Architektur**

Willkommen im Team. Du übernimmst die Arbeit an `quicpro_async`, einem Framework, das die traditionellen Grenzen von PHP sprengt. Unser Ziel ist es, PHP mit der Rohleistung und den Low-Level-Fähigkeiten einer System-Programmiersprache wie Go oder Rust auszustatten, ohne dabei die Produktivität und das riesige Ökosystem von PHP zu opfern.

Vergiss das klassische Request-Response-Modell. Wir bauen langlebige, hochgradig nebenläufige und zustandsbehaftete Netzwerkdienste – von Echtzeit-Anwendungen über IoT-Plattformen bis hin zu komplexen KI-Agenten-Systemen.

Die Architektur ist in logische Schichten unterteilt. Verstehe jede Schicht und ihre Aufgabe, und Du wirst das gesamte System beherrschen.

### Layer 1: Die Konfigurations-Stiftung

Alles beginnt mit der Konfiguration. Die Architektur bietet ein flexibles, dreistufiges System, um ein Maximum an Kontrolle und Sicherheit zu gewährleisten.

1.  **Baseline: `php.ini`**: Auf der untersten Ebene werden systemweite Standardwerte und Sicherheitsrichtlinien in der `php.ini` festgelegt. Hier definiert der Administrator globale Regeln, z. B. die Standardanzahl der Worker-Prozesse, Cluster-Verhalten, oder sicherheitskritische TLS-Vorgaben. Dies ist das Sicherheitsnetz und die Grundlage für alle Applikationen.

2.  **Laufzeit-Anpassung: Das `Quicpro\Config` Objekt**: Für jede einzelne Client-Verbindung oder Server-Instanz können Entwickler die globalen `php.ini`-Einstellungen überschreiben, indem sie ein `Quicpro\Config`-Objekt instanziieren. Dies ermöglicht die Feinabstimmung von Parametern wie Timeouts, Congestion-Control-Algorithmen oder das Laden spezifischer TLS-Zertifikate für eine mTLS-Verbindung.

3.  **Die Administrative Sperre**: Der `php.ini`-Schalter `quicpro.allow_config_override` ist der "Generalschlüssel". Ist er auf `false` gesetzt, kann die Laufzeit-Konfiguration die globalen Vorgaben nicht mehr überschreiben. Jeder Versuch führt zu einer `PolicyViolationException`. Dies ist ein entscheidendes Sicherheitsfeature für Produktionsumgebungen, um kritische Policies zu erzwingen.

### Layer 2: Der C-Kern – Wo die Magie passiert

Das Herz von `quicpro_async` ist ein in C/Rust geschriebener Kern, der als PECL-Extension in die PHP-Runtime integriert ist. Diese Schicht ist für die pure Performance verantwortlich und stellt Low-Level-Systemfunktionen direkt in PHP bereit.

* **Protokoll-Stacks**: Der Kern implementiert die Transportprotokolle. An erster Stelle steht ein nativer **QUIC/HTTP/3-Stack** für moderne, latenzarme und mobile-freundliche Verbindungen. Als Fallback und für universelle Kompatibilität ist ebenfalls ein nativer **TCP-Stack** sowie eine Integration mit **libcurl** für HTTP/1.1 und HTTP/2 vorhanden. Die Konfiguration erlaubt es, TLS komplett zu deaktivieren, was in hochsicheren, internen Netzwerken wie bei KI-Clustern für den letzten Rest an Performance entscheidend sein kann.

* **CPU- und I/O-Tuning**: Für "Bare-Metal"-Performance bietet der Kern direkte Kontrolle über die Hardware-Affinität. Prozesse und I/O-Threads können an spezifische **CPU-Kerne** (z.B. P-Cores vs. E-Cores) und **NUMA-Nodes** gebunden werden, um Cache-Misses und Memory-Latenz zu minimieren. Techniken wie `io_uring`, `SO_BUSY_POLL` und Zero-Copy I/O sind über die Konfiguration zugänglich, um den System-Overhead zu eliminieren.

### Layer 3: Abstraktionen – Server, Client & Concurrency

Aufbauend auf dem C-Kern bietet das Framework elegante High-Level-Abstraktionen, die die Komplexität der Netzwerkprogrammierung verbergen.

* **Der `Server`**: Die `Quicpro\Server`-Klassen sind das Fundament für alle serverseitigen Anwendungen. Statt eines traditionellen Webservers wie Nginx wird die PHP-Applikation selbst zum hochperformanten Server. Der bevorzugte Weg ist die **ereignisgesteuerte `.on()`-API**, bei der man lediglich Callbacks für Events wie `connect`, `message` und `close` registriert.

* **Concurrency mit Fibers**: Jede eingehende Client-Verbindung wird automatisch in einer eigenen **PHP Fiber** behandelt. Dieses "Fiber-per-Client"-Modell wird vom C-Kern verwaltet und ermöglicht massive Nebenläufigkeit innerhalb eines einzigen PHP-Prozesses. Langsame Operationen in einer Fiber (z.B. ein DB-Query) blockieren niemals die Abarbeitung von tausenden anderen Clients.

* **Native CORS-Behandlung**: Sicherheitsfeatures wie CORS sind direkt im C-Kern des Servers implementiert. Die Policy wird im `Config`-Objekt definiert, und der Server erzwingt sie, bevor der PHP-Code überhaupt eine Anfrage zu Gesicht bekommt. Das ist schnell und sicher.

### Layer 4: Service-Kommunikation – MCP & IIBIN

Für die Kommunikation zwischen den Microservices (Agenten) haben wir einen eigenen, hochoptimierten Stack entwickelt.

* **MCP (Model Context Protocol)**: MCP ist der einheitliche Kommunikationsbus für alle internen Dienste. Es ist ein leichtgewichtiger RPC-Mechanismus, der über QUIC läuft und es Diensten ermöglicht, klar definierte Methoden bei anderen Diensten aufzurufen.

* **IIBIN**: Das Standard-Serialisierungsformat ist **IIBIN**, ein an Protobuf angelehntes, schema-basiertes Binärformat. Es ist extrem performant, da die (De-)Serialisierung im C-Kern stattfindet. Schemata werden einmalig definiert und garantieren typsichere und validierte Datenübertragung. Für maximale Flexibilität und einfache Integration, z.B. mit externen Tools, ist geplant, **JSON** als alternatives Payload-Format für MCP zuzulassen.

### Layer 5: Die Krönung – Der PipelineOrchestrator

Dies ist eine der mächtigsten Komponenten des Frameworks. Der `PipelineOrchestrator` ist eine in C implementierte Engine zur Ausführung von komplexen, mehrstufigen Workflows.

* **Deklarative Workflows**: Statt komplexe, prozedurale Logik in PHP zu schreiben, wird ein Workflow als einfache, deklarative Datenstruktur (ein PHP-Array) definiert. Diese Definition beschreibt die einzelnen Schritte, ihre Abhängigkeiten und den Datenfluss zwischen ihnen.

* **Tool-Handler & MCP**: Die einzelnen Schritte in einer Pipeline sind generische "Tools" (z.B. `GenerateText`, `FetchUrlContent`). Diese Tools werden über `registerToolHandler` an spezifische MCP-Agenten gebunden. Der Orchestrator kümmert sich um die gesamte Kommunikation, Serialisierung und Fehlerbehandlung.

* **Anwendungsfall**: Dies ermöglicht die Erstellung extrem komplexer Anwendungen, wie z.B. eines KI-Agenten, der einen internen "Gedankenprozess" als Pipeline abbildet, externe Tools über MCP-Agenten aufruft und auf deren Ergebnisse reagiert – alles mit minimalem PHP-Code.

====

### **Die Komponenten im Detail**

Nachdem Du nun die grundlegenden Architekturschichten verstanden hast, wollen wir uns die einzelnen Bausteine genauer ansehen. Jede Komponente hat eine spezifische Aufgabe und interagiert mit den anderen, um das Gesamtsystem zu bilden.

### Client

Der **Client** ist die Komponente, die ausgehende Verbindungen zu Servern herstellt und mit ihnen interagiert. Im `quicpro_async`-Framework ist die primäre Abstraktion hierfür die `Quicpro\Session`-Klasse oder die prozedurale Funktion `quicpro_connect()`. Der Client ist weit mehr als ein einfacher Socket-Wrapper; er ist das Tor zum leistungsstarken C-Kern und dessen Netzwerk-Features.

**Kernfunktionalitäten:**

* **Protokoll-Vielfalt**: Der Client kann über QUIC/HTTP/3, aber auch über die Fallbacks HTTP/2 und HTTP/1.1 kommunizieren. Die Konfiguration bestimmt, welche Protokolle versucht werden.
* **Transparente Komplexität**: Der Client abstrahiert die gesamte Komplexität des Verbindungsaufbaus, des TLS-Handshakes und der Fehlerbehandlung.
* **Resilienz durch Connection Migration**: Eine der herausragendsten Fähigkeiten des QUIC-Clients ist die automatische "Connection Migration". Wechselt ein mobiler Client das Netzwerk (z.B. von Mobilfunk zu WLAN), bleibt die bestehende Verbindung nahtlos erhalten, ohne dass die Anwendung eingreifen muss. Dies ist entscheidend für IoT-Anwendungen wie das Flotten-Tracking.
* **Nebenläufigkeit**: Der Client ist für den nebenläufigen Betrieb in Fibers ausgelegt. Du kannst tausende ausgehende Anfragen parallel aus einem einzigen PHP-Prozess initiieren, ohne dass eine Anfrage die andere blockiert.

**Beispiel: Parallele API-Abfragen mit Fibers**

Dieses Beispiel zeigt, wie man drei API-Endpunkte gleichzeitig abfragt. Jede Anfrage läuft in einer eigenen Fiber, und das Hauptprogramm wartet, bis alle abgeschlossen sind. Dies ist das fundamentale Muster für performante Fan-Out-Operationen.

~~~php
<?php
// Holt den Inhalt eines Pfades von einem internen Dienst.
// Die Funktion ist so konzipiert, dass sie in einer Fiber läuft.
function fetch(string $path): string 
{
    // Konfiguration für den Client (kann aus einer zentralen Stelle kommen)
    $config = Quicpro\Config::new(['verify_peer' => true, 'alpn' => ['h3']]);
    
    // Baut eine QUIC-Verbindung zum Server auf.
    $session = new Quicpro\Session('api.internal.service', 443, $config);
    
    // Sendet die Anfrage auf einem neuen Stream. Dies ist ein nicht-blockierender Aufruf.
    $stream = $session->sendRequest('GET', $path);

    // Die "magische" Schleife: Wir pollen die Session, bis eine Antwort eintrifft.
    // quicpro_poll() gibt die Kontrolle an den C-Kern ab, der die Netzwerkarbeit erledigt.
    // Der Timeout von 5ms sorgt für minimale Latenz, ohne die CPU zu verbrennen.
    while (!$response = $stream->receiveResponse()) {
        $session->poll(5);
    }
    
    // Sobald die Antwort da ist, wird die Verbindung geschlossen und der Body zurückgegeben.
    $session->close();
    return $response->getBody();
}

// Die Liste der Aufgaben, die parallel ausgeführt werden sollen.
$jobs = ['/users/1', '/products/42', '/orders/latest'];

// Für jede Aufgabe wird eine neue Fiber mit unserer fetch-Funktion erstellt.
$fibers = array_map(
    fn($path) => new Fiber(fn() => fetch($path)), 
    $jobs
);

// Alle Fibers werden gestartet. PHP führt sie kooperativ aus.
foreach ($fibers as $fiber) {
    $fiber->start();
}

// Wir warten auf die Ergebnisse jeder einzelnen Fiber und geben sie aus.
// getReturn() blockiert nur, bis die jeweilige Fiber fertig ist.
foreach ($fibers as $index => $fiber) {
    echo "Result from {$jobs[$index]}: ". $fiber->getReturn(). "\n";
}
?>
~~~

### Cluster

Der **`Quicpro\Cluster`** ist die Komponente für vertikale Skalierung und Resilienz auf einem einzelnen Server. Es ist ein im C-Kern implementierter Prozess-Supervisor, der es einer PHP-Anwendung ermöglicht, alle verfügbaren CPU-Kerne effizient zu nutzen. Er ist die Antwort auf das traditionelle "ein Prozess pro Anfrage"-Limit von PHP-FPM in langlebigen Anwendungen.

**Kernfunktionalitäten:**

* **Multi-Prozess-Architektur**: Der Cluster startet eine Master-Anwendung, die dann eine konfigurierbare Anzahl von identischen Worker-Prozessen (`fork`s) erzeugt. Die `php.ini`-Einstellung `quicpro.workers = 0` sorgt dafür, dass automatisch ein Worker pro CPU-Kern gestartet wird.
* **Kernel-Level Load Balancing**: Alle Worker lauschen auf demselben Port (z.B. UDP 4433) unter Verwendung der `SO_REUSEPORT`-Socket-Option. Der Betriebssystem-Kernel verteilt eingehende Verbindungen effizient und fair auf alle lauschenden Prozesse. Es ist kein externer Load Balancer auf der Maschine erforderlich.
* **Self-Healing**: Der Master-Prozess überwacht den Zustand seiner Worker. Stürzt ein Worker ab, startet der Cluster ihn automatisch neu, was die Anwendung hochverfügbar und resilient gegen unerwartete Fehler macht.
* **Shared-Memory Ticket Cache**: In Kombination mit dem Cluster kann ein Shared-Memory-basierter TLS-Session-Ticket-Cache aktiviert werden. Dies ermöglicht 0-RTT-Verbindungen, selbst wenn ein Client bei einer erneuten Verbindung von einem anderen Worker-Prozess bedient wird.

**Beispiel: Ein Multi-Worker WebSocket Server**

Dieses Beispiel zeigt, wie man einen Server startet, der auf jedem CPU-Kern einen Worker-Prozess ausführt. Jeder Worker ist ein vollwertiger, nebenläufiger WebSocket-Server.

~~~php
<?php
declare(strict_types=1);

use Quicpro\Config;
use Quicpro\Cluster;

// Die Konfiguration für den Cluster.
// Wir lassen die Anzahl der Worker automatisch erkennen.
$clusterConfig = Config::new([
    'cluster_workers' => 0, // Auto-detect CPU cores
    'cluster_restart_crashed_workers' => true,
]);

// Das ist das Skript, das jeder einzelne Worker-Prozess ausführen wird.
$workerScript = __DIR__ . '/my_websocket_worker.php';

// Die Nutzlast, die an jeden Worker übergeben wird (z.B. Konfigurationsdaten).
$payload = json_encode(['listen_port' => 4433]);

// Instanziiere und starte den Cluster.
// Der aktuelle Prozess wird zum Master, der die Worker überwacht.
$cluster = new Cluster($workerScript, $payload, $clusterConfig);
echo "Master process starting. Spawning workers...\n";
$cluster->run(); // Dieser Aufruf blockiert und startet die Überwachungsschleife.

// --- Inhalt von my_websocket_worker.php ---
/*
<?php
use Quicpro\Config;
use Quicpro\WebSocket\Server;
use Quicpro\WebSocket\Connection;

// Jeder Worker erhält seine Konfiguration über Umgebungsvariablen.
$workerId = (int)getenv('QUICPRO_WORKER_ID');
$payload = json_decode(getenv('QUICPRO_PAYLOAD'), true);
$port = $payload['listen_port'];

$serverConfig = Config::new([
    'cert_file' => './certs/server.pem',
    'key_file'  => './certs/server.key',
    'session_cache' => true, // Wichtig für 0-RTT über Worker hinweg
]);

$server = new Server('0.0.0.0', $port, $serverConfig); // SO_REUSEPORT ist implizit
echo "[Worker {$workerId}] listening on port {$port}\n";

$server->on('message', function(Connection $conn, string $msg) use ($workerId) {
    $conn->send("Hello from worker #{$workerId}: you said '{$msg}'");
});

$server->run();
?>
*/
~~~

### Config

Die **`Quicpro\Config`**-Klasse ist das zentrale Steuerungselement für das Verhalten von Servern und Clients. Es ist ein unveränderliches Konfigurationsobjekt ("immutable value object"), das es ermöglicht, das Verhalten jeder einzelnen Verbindung oder Serverinstanz präzise zu steuern und dabei die globalen `php.ini`-Vorgaben zu überschreiben (sofern erlaubt).

**Kernfunktionalitäten:**

* **Typsichere Konfiguration**: Das Erstellen eines `Config`-Objekts ist streng validiert. Die Verwendung eines unbekannten Schlüssels wirft eine `UnknownConfigOptionException`, ein falscher Datentyp eine `InvalidConfigValueException`. Dies verhindert Konfigurationsfehler zur Laufzeit.
* **Granulare Kontrolle**: Fast jeder Aspekt des C-Kerns kann über dieses Objekt gesteuert werden – von TLS-Parametern (`verify_peer`, `ca_file`) über QUIC-Transportparameter (`cc_algorithm`, `max_idle_timeout_ms`) bis hin zu experimentellen Performance-Features (`busy_poll_duration_us`).
* **Sicherheits-Policy**: Es ist das primäre Werkzeug zur Definition von Sicherheitsrichtlinien für eine Verbindung, wie z.B. die Anforderung von Mutual TLS (mTLS) durch Setzen von `cert_file` und `key_file`.

**Beispiel: Konfiguration für einen mTLS-gesicherten MCP-Client**

Dieses Beispiel zeigt, wie ein Client konfiguriert wird, der sich bei einem internen Dienst authentifizieren muss, indem er sein eigenes Zertifikat vorzeigt.

~~~php
<?php
use Quicpro\Config;
use Quicpro\MCP;

try {
    // Erstelle ein Config-Objekt für einen sicheren internen Client.
    $mcpClientConfig = Config::new([
        // Validiere das Zertifikat des Servers anhand unserer internen CA.
        'verify_peer' => true,
        'ca_file'     => './ca/internal_ca.pem',
        
        // Präsentieren unser eigenes Zertifikat zur Authentifizierung.
        'cert_file'   => './certs/client_service.pem',
        'key_file'    => './certs/client_service.key',
        
        // Definiere das Anwendungsprotokoll.
        'alpn'        => ['mcp/1.0'],
        
        // Setze ein aggressives Timeout für schnelle Fehlererkennung.
        'max_idle_timeout_ms' => 5000,
    ]);

    // Der MCP-Client verwendet dieses Konfigurationsobjekt.
    $secureClient = new MCP('secure-service.internal', 9901, $mcpClientConfig);
    
    echo "Secure MCP client configured and ready.\n";
    // $response = $secureClient->request(...);

} catch (\Quicpro\Exception\ConfigException $e) {
    die("Configuration Error: " . $e->getMessage());
}
?>
~~~

### Connect

Die **`connect`**-Funktionalität ist der Einstiegspunkt für jede ausgehende Client-Verbindung. Sie wird entweder durch die prozedurale Funktion `quicpro_connect()` oder implizit durch den Konstruktor von High-Level-Klassen wie `Quicpro\Session` oder `Quicpro\MCP` aufgerufen. Diese Komponente ist für die Auflösung des Hostnamens und den Aufbau der zugrunde liegenden UDP-Verbindung verantwortlich.

**Kernfunktionalitäten:**

* **Happy Eyeballs (RFC 8305)**: In Dual-Stack-Netzwerken (IPv4 & IPv6) versucht `connect` standardmäßig, eine IPv6-Verbindung herzustellen, fällt aber nach einer kurzen, konfigurierbaren Verzögerung (`happy_eyeballs_delay_ms`) auf IPv4 zurück. Dies minimiert die Verbindungsaufbauzeit in Umgebungen, in denen IPv6 beworben, aber nicht korrekt geroutet wird.
* **Adressfamilien-Kontrolle**: Du kannst explizit erzwingen, nur IPv4 oder IPv6 zu verwenden, was in containerisierten Umgebungen oder für deterministisches Routing wichtig ist.
* **Interface-Bindung**: Ermöglicht das Binden des ausgehenden Sockets an ein bestimmtes Netzwerk-Interface (z.B. `"eth0"`), was für Multi-Homed-Server oder spezielle Routing-Anforderungen entscheidend ist.

**Beispiel: Verbindungsaufbau mit optimiertem Happy Eyeballs**

Dieses Beispiel zeigt, wie man eine Verbindung mit einer sehr kurzen Verzögerung für den IPv4-Fallback herstellt, um in latenzkritischen Umgebungen den Verbindungsaufbau zu beschleunigen.

~~~php
<?php
use Quicpro\Config;
use Quicpro\Session;

// Standardkonfiguration
$config = Quicpro\Config::new([
    'verify_peer' => true,
    'alpn' => ['h3'],
]);

// Optionen für den Verbindungsaufbau
$connectOptions = [
    'family' => 'auto', // Standard: Versuche IPv6, dann IPv4
    'happy_eyeballs_delay_ms' => 10, // Sehr kurze Verzögerung von 10ms
];

try {
    // Die Optionen werden als vierter Parameter an quicpro_connect
    // oder als dritter Parameter an den Session-Konstruktor übergeben.
    $session = new Session('example.com', 443, $config, $connectOptions);
    
    if ($session->isConnected()) {
        echo "Successfully connected to example.com with optimized Happy Eyeballs.\n";
        $session->close();
    }

} catch (\Quicpro\Exception\ConnectionException $e) {
    die("Connection failed: " . $e->getMessage());
}
?>
~~~

### CORS (Cross-Origin Resource Sharing)

Die **CORS**-Behandlung ist als performante, native Funktionalität direkt in den C-Kern des `Quicpro\Server` integriert. Dies ermöglicht es dem Server, Cross-Origin-Anfragen von Web-Frontends sicher und mit minimalem Overhead zu bearbeiten, ohne dass PHP-Userland-Code involviert werden muss.

**Kernfunktionalitäten:**

* **Native Performance**: Da die CORS-Prüfung und die Beantwortung von `OPTIONS`-Preflight-Anfragen im C-Kern stattfinden, geschieht dies mit maximaler Geschwindigkeit, bevor die Anfrage überhaupt die PHP-Ebene erreicht.
* **Zentrale Konfiguration**: Die CORS-Policy wird einfach im `Quicpro\Config`-Objekt des Servers mit dem Schlüssel `cors_allowed_origins` als kommagetrennte Liste von erlaubten Domains (oder `*`) definiert.
* **Automatische Preflight-Behandlung**: Der Server erkennt `OPTIONS`-Preflight-Anfragen automatisch und antwortet mit den korrekten Headern (`Access-Control-Allow-Origin`, `Access-Control-Allow-Methods`, etc.), ohne dass Du dafür Code schreiben musst.
* **Fallback auf Userland**: Wenn `cors_allowed_origins` auf `false` gesetzt wird, wird der native Handler deaktiviert, was es Dir ermöglicht, komplexe, dynamische CORS-Logiken (z.B. basierend auf Datenbankabfragen) vollständig im PHP-Code zu implementieren.

**Beispiel: Server mit nativer CORS-Policy**

Dieses Beispiel startet einen WebSocket-Server, der nur Verbindungen von `https://my-app.com` und `http://localhost:8080` erlaubt.

~~~php
<?php
use Quicpro\Config;
use Quicpro\WebSocket\Server as WebSocketServer;

// Die CORS-Policy wird direkt in der Konfiguration definiert.
$serverConfigWithCors = Config::new([
    'cert_file' => './certs/server.pem',
    'key_file'  => './certs/server.key',
    'alpn'      => ['h3'],
    
    // Erlaube nur diese beiden Origins.
    'cors_allowed_origins' => 'https://my-app.com,http://localhost:8080',
]);

$wsServer = new WebSocketServer('0.0.0.0', 4433, $serverConfigWithCors);

$wsServer->on('connect', function ($conn) {
    echo "Client connected. Native CORS check passed.\n";
    $conn->send("Welcome!");
});

echo "WebSocket server with native CORS policy is running.\n";
$wsServer->run();
?>
~~~

### HTTP/2, HTTP/3, HTTP Client

Das Framework bietet umfassende Unterstützung für die modernen HTTP-Protokolle und stellt einen leistungsfähigen, einheitlichen **HTTP-Client** zur Verfügung, der die Komplexität der verschiedenen Protokollversionen abstrahiert.

**Kernfunktionalitäten:**

* **HTTP/3 First**: `quicpro_async` ist primär auf **HTTP/3** ausgelegt und nutzt dafür seinen nativen QUIC-Stack. Dies ist der Standard für alle Client- und Server-Operationen, um von minimaler Latenz und Connection Migration zu profitieren.
* **HTTP/2 & HTTP/1.1 Fallback**: Für die Kompatibilität mit älteren Servern oder Infrastrukturen integriert das Framework `libcurl`, um einen robusten und voll funktionsfähigen Client für **HTTP/2** und **HTTP/1.1** bereitzustellen.
* **Einheitliche Client-API**: Die Komplexität des Protokoll-Fallbacks ist für den Entwickler transparent. High-Level-Klassen wie `Quicpro\MCP` oder ein zukünftiger `Quicpro\HttpClient` würden intern automatisch das beste verfügbare Protokoll aushandeln, ohne dass der aufrufende Code geändert werden muss.
* **Server-Unterstützung**: Der `Quicpro\Server` kann ebenfalls so konfiguriert werden, dass er neben QUIC/HTTP/3 auch TCP-basierte Verbindungen für HTTP/2 und HTTP/1.1 akzeptiert, um als universeller Webserver zu fungieren.

**Beispiel: Zukünftiger High-Level HTTP Client (Konzept)**

Dieses konzeptionelle Beispiel zeigt, wie eine zukünftige, einheitliche Client-API aussehen könnte, die automatisch das beste Protokoll wählt.

~~~php
<?php
use Quicpro\HttpClient;
use Quicpro\Config;

// Konfiguration, die alle Protokolle erlaubt.
$clientConfig = Config::new([
    'verify_peer' => true,
    // Der Client wird versuchen, h3 auszuhandeln, und bei Fehlschlag
    // über den libcurl-Pfad h2 oder http/1.1 versuchen.
    'alpn' => ['h3', 'h2', 'http/1.1'], 
]);

$client = new HttpClient($clientConfig);

try {
    // Mache eine Anfrage an einen Server, der möglicherweise nur HTTP/2 unterstützt.
    // Der Client würde den QUIC-Handshake versuchen, scheitern und dann
    // nahtlos einen TCP-basierten Versuch mit libcurl starten.
    $response = $client->get('https://some-legacy-api.com/data');
    
    echo "Successfully fetched data using protocol: " . $response->getProtocolVersion() . "\n";
    echo "Body: " . $response->getBody() . "\n";

} catch (\Quicpro\Exception\ConnectionException $e) {
    die("Failed to connect using any available protocol: " . $e->getMessage());
}
?>
~~~

### IIBIN (Intelligent Intern Binary)

**IIBIN** ist das native Serialisierungsformat von `quicpro_async`, das für maximale Performance und Typsicherheit bei der Kommunikation zwischen Microservices (Agenten) entwickelt wurde. Es ist stark an Google's Protocol Buffers (Protobuf) angelehnt und wird vollständig im C-Kern verarbeitet.

**Kernfunktionalitäten:**

* **Schema-basiert**: Bevor Daten gesendet oder empfangen werden können, muss ein Schema definiert werden. Dieses Schema ist der "Vertrag" zwischen Client und Server und beschreibt die Struktur, die Feldnamen und die Datentypen der Nachricht.
* **Extrem performant**: Die Kodierung von PHP-Arrays in einen binären IIBIN-Payload und die Dekodierung zurück erfolgen nativ in C. Dies ist um Größenordnungen schneller als `json_encode`/`json_decode` oder `serialize`/`unserialize` in PHP.
* **Kompakte Payloads**: Das Binärformat ist platzsparend, was die Netzwerklast reduziert – ein entscheidender Vorteil in hochfrequenten Systemen.
* **Typsicherheit und Validierung**: Die schemabasierte Natur stellt sicher, dass nur valide Daten gesendet und empfangen werden. Jeder Versuch, Daten zu kodieren, die nicht dem Schema entsprechen, führt zu einem Fehler, was die Robustheit des Gesamtsystems erhöht.

**Beispiel: Definition und Verwendung eines IIBIN-Schemas**

Dieses Beispiel zeigt, wie man ein Schema für eine Telemetrie-Nachricht definiert und diese dann kodiert und dekodiert.

~~~php
<?php
use Quicpro\IIBIN;

// 1. Definiere das Schema für eine Telemetrie-Nachricht.
// Dies geschieht einmalig beim Start der Anwendung.
IIBIN::defineSchema('GeoTelemetry', [
    'vehicle_id' => ['type' => 'string', 'tag' => 1],
    'lat'        => ['type' => 'double', 'tag' => 2],
    'lon'        => ['type' => 'double', 'tag' => 3],
    'speed_kmh'  => ['type' => 'uint32', 'tag' => 4, 'optional' => true],
    'timestamp'  => ['type' => 'uint64', 'tag' => 5],
]);

// 2. Erzeuge ein PHP-Array mit den Daten.
$telemetryData = [
    'vehicle_id' => 'TRUCK-042',
    'lat'        => 48.8584,
    'lon'        => 2.2945,
    'timestamp'  => time(),
];

// 3. Kodiere das Array in einen binären Payload.
$binaryPayload = IIBIN::encode('GeoTelemetry', $telemetryData);
echo "Encoded binary payload (length: " . strlen($binaryPayload) . " bytes)\n";

// (Dieser Payload würde nun über das Netzwerk gesendet werden)

// 4. Dekodiere den binären Payload auf der Empfängerseite zurück in ein PHP-Array.
$decodedData = IIBIN::decode('GeoTelemetry', $binaryPayload);

print_r($decodedData);
?>
~~~

### MCP (Model Context Protocol)

**MCP** ist das von uns entwickelte, leichtgewichtige RPC-Protokoll (Remote Procedure Call), das als einheitlicher Kommunikationsbus für alle internen Microservices (Agenten) dient. Es ist speziell für die Verwendung über QUIC konzipiert und nutzt IIBIN für die Serialisierung, um eine hochperformante und robuste Service-zu-Service-Kommunikation zu ermöglichen.

**Kernfunktionalitäten:**

* **Service- und Methoden-Routing**: MCP ermöglicht es einem Client, eine bestimmte Methode auf einem bestimmten Service auf einem Remote-Server aufzurufen. Der `Quicpro\MCP\Server` leitet eingehende Anfragen automatisch an die richtige, registrierte PHP-Klassenmethode weiter.
* **Abstraktion**: Es abstrahiert die Komplexität der Netzwerkkommunikation. Als Entwickler rufst Du einfach eine Methode auf einem Client-Objekt auf (`$client->request(...)`), und das Framework kümmert sich um den Verbindungsaufbau, die Serialisierung, das Senden der Anfrage und das Warten auf die Antwort.
* **Einheitlichkeit**: Alle internen Dienste sprechen dieselbe "Sprache". Dies vereinfacht die Entwicklung, das Debugging und die Wartung des Gesamtsystems erheblich.
* **Tool-Integration**: MCP ist die Grundlage für den `PipelineOrchestrator`. Jeder "Tool"-Aufruf in einer Pipeline wird intern in einen MCP-Request an den entsprechenden Agenten übersetzt.

**Beispiel: Ein einfacher MCP-Server und -Client**

Dieses Beispiel zeigt einen "Persistence Agent", der eine Methode `recordTelemetry` anbietet, und einen Client, der diese Methode aufruft.

~~~php
<?php
// --- persistence_agent.php (Server) ---
use Quicpro\Config;
use Quicpro\MCP\Server as McpServer;
use Quicpro\IIBIN;

// Schemata definieren...
IIBIN::defineSchema('GeoTelemetry', [/*...*/]);
IIBIN::defineSchema('Ack', ['status' => ['type' => 'string', 'tag' => 1]]);

class PersistenceService {
    public function recordTelemetry(string $payload): string {
        $data = IIBIN::decode('GeoTelemetry', $payload);
        // ... Logik zum Speichern der Daten in der DB ...
        echo "Received telemetry for: " . $data['vehicle_id'] . "\n";
        return IIBIN::encode('Ack', ['status' => 'OK']);
    }
}

$server = new McpServer('127.0.0.1', 9601, Config::new());
$server->registerService('PersistenceService', new PersistenceService());
echo "Persistence Agent listening...\n";
$server->run();


// --- ingestion_gateway.php (Client) ---
use Quicpro\Config;
use Quicpro\MCP;
use Quicpro\IIBIN;

// Schemata müssen auch hier definiert sein...
IIBIN::defineSchema('GeoTelemetry', [/*...*/]);
IIBIN::defineSchema('Ack', ['status' => ['type' => 'string', 'tag' => 1]]);

$telemetryData = ['vehicle_id' => 'TRUCK-007', /*...*/];
$payload = IIBIN::encode('GeoTelemetry', $telemetryData);

$persistenceClient = new MCP('127.0.0.1', 9601, Config::new());

// Rufe die Methode 'recordTelemetry' auf dem 'PersistenceService' auf.
$responsePayload = $persistenceClient->request(
    'PersistenceService', 
    'recordTelemetry', 
    $payload
);

$ack = IIBIN::decode('Ack', $responsePayload);
echo "Server response: " . $ack['status'] . "\n";
?>
~~~

### Pipeline Orchestrator

Der **`PipelineOrchestrator`** ist die intelligenteste und abstrakteste Komponente des Frameworks. Es ist eine in C implementierte Engine, die komplexe, mehrstufige Workflows ausführt, die als einfache PHP-Arrays definiert werden. Er verwandelt prozedurale Logik in deklarative Datenstrukturen und ist das Herzstück für die Implementierung von KI-Agenten und komplexen Datenverarbeitungspipelines.

**Kernfunktionalitäten:**

* **Deklarative Workflows**: Statt die Logik in PHP auszuprogrammieren, beschreibst Du sie als eine Liste von Schritten. Dies macht die Logik lesbarer, wartbarer und einfacher zu analysieren.
* **Tool-basiert**: Jeder Schritt in der Pipeline ist ein Aufruf an ein generisches "Tool" (z.B. `GenerateText`, `FetchMarketDataHFT`). Diese Tools werden einmalig über `registerToolHandler` an konkrete MCP-Agenten gebunden. Der Orchestrator ist der "Projektleiter", der die verschiedenen "Handwerker" (MCP-Agenten) koordiniert.
* **Datenfluss-Management**: Der Orchestrator verwaltet den Datenfluss zwischen den Schritten. Du kannst den Output eines Schrittes (`@previous.output`) oder den Output eines spezifischen, früheren Schrittes (`@StepID.output`) als Input für einen späteren Schritt verwenden.
* **Bedingte Logik und Parallelisierung**: Die Pipeline-Definition unterstützt bedingte Ausführung (`'if' => '...'`) und die parallele Ausführung von Schritten (`'type' => 'parallel'`), was die C-Engine nutzt, um die Operationen nebenläufig auszuführen.

**Beispiel: Eine einfache Datenanreicherungs-Pipeline**

Diese Pipeline holt Benutzerdaten, extrahiert den Domain-Namen aus der E-Mail und ruft dann einen externen Dienst auf, um Informationen über diese Domain zu erhalten.

~~~php
<?php
use Quicpro\PipelineOrchestrator;

// Annahme: Die Tools 'FetchUserFromDB', 'ExtractDomain' (ein einfacher String-Manipulations-Tool)
// und 'EnrichDomainInfo' (ein MCP-Agent) wurden bereits registriert.

$initialData = ['user_id' => 123];

$pipelineDefinition = [
    [
        'id' => 'FetchUser',
        'tool' => 'FetchUserFromDB',
        'input_map' => ['id' => '@initial.user_id'], 
        // Output könnte sein: { "name": "John Doe", "email": "john.doe@example.com" }
    ],
    [
        'id' => 'GetDomain',
        'tool' => 'ExtractDomain', // Ein einfaches, lokales Tool
        'input_map' => ['email_address' => '@FetchUser.email'],
        // Output könnte sein: { "domain": "example.com" }
    ],
    [
        'id' => 'EnrichDomain',
        'tool' => 'EnrichDomainInfo', // Ein MCP-Tool
        'input_map' => ['domain_name' => '@GetDomain.domain'],
        // Output könnte sein: { "company_name": "Example Inc.", "founded_year": 1999 }
    ]
];

$result = PipelineOrchestrator::run($initialData, $pipelineDefinition);

if ($result->isSuccess()) {
    print_r($result->getFinalOutput()); // Würde den Output von 'EnrichDomain' enthalten
} else {
    echo "Pipeline failed: " . $result->getErrorMessage() . "\n";
}
?>
~~~

### poll

Die **`poll`**-Funktionalität, verfügbar als Methode `Quicpro\Session::poll()` oder prozedurale Funktion `quicpro_poll()`, ist der Herzschlag jeder `quicpro_async`-Anwendung, die im manuellen Modus betrieben wird. In der ereignisgesteuerten API (`.on(...)`) wird sie vom C-Kern intern aufgerufen. Wenn Du jedoch eine `while`-Schleife selbst verwaltest, ist der Aufruf von `poll` unerlässlich.

**Kernfunktionalitäten:**

* **Antreiben der State Machine**: Ein Aufruf an `poll` übergibt die Kontrolle an den C-Kern. Der Kern nutzt diese Zeit, um alle anstehenden Netzwerk-Aufgaben zu erledigen: Er liest ankommende Pakete von den Sockets, verarbeitet sie (z.B. entschlüsselt TLS-Daten, reassembliert QUIC-Streams), sendet ausgehende Pakete aus seinen Puffern und managt alle Timer (z.B. für Timeouts oder Retransmissions).
* **Kooperatives Multitasking**: Der `timeout`-Parameter von `poll($session, $timeout_ms)` gibt an, wie lange der C-Kern maximal auf neue Netzwerkereignisse warten soll. Dies ist der Mechanismus, durch den Fibers kooperativ arbeiten. Ein Aufruf von `poll(5)` bedeutet: "Lieber C-Kern, erledige deine Netzwerkarbeit. Wenn du nichts zu tun hast, warte bis zu 5 Millisekunden auf neue Pakete. Danach gib die Kontrolle an mich (PHP) zurück, damit ich andere Fibers ausführen kann.".
* **Vermeidung von Blockaden**: Ohne regelmäßige `poll`-Aufrufe würde eine `while`-Schleife, die auf eine Antwort wartet, die CPU zu 100% auslasten und den gesamten PHP-Prozess blockieren. `poll` ist der "Atempunkt", der es dem System ermöglicht, nebenläufig zu bleiben.

**Beispiel: Manuelle Implementierung eines `fetch`-Clients**

Dieses Beispiel zeigt, wie die `fetch`-Funktion aus dem "Client"-Beispiel ohne die High-Level-Methoden `sendRequest` und `receiveResponse` aussehen würde, um die Rolle von `poll` zu verdeutlichen.

~~~php
<?php
function manual_fetch(string $path): string 
{
    $session = new Quicpro\Session('example.org', 443, Quicpro\Config::new());
    
    // Erzeuge einen neuen Stream manuell.
    $streamId = $session->openBidirectionalStream();
    
    // Sende die HTTP-Header auf diesem Stream.
    $headers = [[':method', 'GET'], [':path', $path], [':scheme', 'https'], [':authority', 'example.org']];
    $session->sendHeaders($streamId, $headers, true); // true = FIN (Ende der Anfrage)

    $responseBody = '';
    $headersReceived = false;

    // Die manuelle Schleife, die auf die Antwort wartet.
    while (true) {
        // Der entscheidende Aufruf! Ohne ihn passiert nichts.
        $session->poll(10); 
        
        // Lese alle anstehenden Ereignisse für unseren Stream.
        foreach ($session->streamEvents($streamId) as $event) {
            if ($event['type'] === 'headers') {
                // Header sind angekommen.
                $headersReceived = true;
            } elseif ($event['type'] === 'body_chunk') {
                // Ein Teil des Bodys ist angekommen.
                $responseBody .= $event['data'];
            } elseif ($event['type'] === 'finished') {
                // Der Stream wurde vom Server beendet. Wir sind fertig.
                $session->close();
                return $responseBody;
            }
        }

        if (!$session->isConnected()) {
            throw new \RuntimeException("Connection lost");
        }
    }
}

echo manual_fetch('/');
?>
~~~

### Server

Der **`Quicpro\Server`** ist die Kernkomponente für die Erstellung von langlebigen, serverseitigen Anwendungen. Er ersetzt traditionelle Webserver wie Nginx oder Apache FPM für `quicpro_async`-Anwendungen, indem er die PHP-Anwendung selbst in einen hochperformanten, nebenläufigen Netzwerkdienst verwandelt.

**Kernfunktionalitäten:**

* **Protokoll-agnostisch**: Die Basis-`Server`-Klasse ist protokoll-agnostisch. Sie kümmert sich um das Binden an einen Port, das Akzeptieren von rohen QUIC-Verbindungen und die Verwaltung von Sessions.
* **Spezialisierte Implementierungen**: Darauf aufbauend gibt es spezialisierte Server-Klassen wie `Quicpro\WebSocket\Server` und `Quicpro\MCP\Server`, die die protokollspezifische Logik kapseln und eine einfachere, auf den Anwendungsfall zugeschnittene API bieten.
* **Ereignisgesteuerte API**: Der bevorzugte Weg zur Interaktion mit dem Server ist die `.on()`-API. Du registrierst Callbacks für Ereignisse, und der C-Kern ruft diese auf, wenn das Ereignis eintritt. Dies führt zu sauberem, reaktivem Code.
* **Fiber-per-Client-Modell**: Jede akzeptierte Verbindung wird automatisch in einer eigenen Fiber behandelt. Dies ermöglicht es dem Server, zehntausende von Clients gleichzeitig zu bedienen, ohne dass eine langsame Operation bei einem Client die anderen blockiert.

**Beispiel: Ein MCP-Server mit der `.on()`-API (Konzept)**

Dieses Beispiel zeigt, wie ein `Quicpro\MCP\Server` konzeptionell mit einer `.on()`-API aussehen könnte, um die ereignisgesteuerte Natur zu verdeutlichen.

~~~php
<?php
use Quicpro\Config;
use Quicpro\MCP\Server as McpServer;
use Quicpro\MCP\Request;

// Annahme: Es gäbe eine McpServer-Klasse mit .on()-Methoden.
$mcpServer = new McpServer('127.0.0.1', 9901, Config::new());

// Registriere einen Handler für eine bestimmte Methode.
$mcpServer->on('PersistenceService:recordTelemetry', function (Request $request) {
    $data = $request->getPayloadDecoded('GeoTelemetry');
    // ... Daten speichern ...
    echo "Telemetry recorded for " . $data['vehicle_id'] . "\n";
    
    // Sende eine Antwort.
    $request->sendResponse(['status' => 'OK'], 'Ack');
});

// Ein generischer Handler für nicht gefundene Methoden.
$mcpServer->on('error:method_not_found', function (Request $request) {
    $request->sendErrorResponse("Method " . $request->getMethodName() . " not found.");
});

echo "Event-driven MCP server is running...\n";
$mcpServer->run();
?>
~~~

### Session

Die **`Quicpro\Session`**-Klasse repräsentiert eine einzelne, aktive QUIC-Verbindung zwischen einem Client und einem Server. Sie ist das zentrale Objekt, über das alle Interaktionen auf einer bestehenden Verbindung stattfinden. Egal ob Du Client oder Server bist, sobald eine Verbindung etabliert ist, arbeitest Du mit einem `Session`-Objekt.

**Kernfunktionalitäten:**

* **Zustandsverwaltung**: Das `Session`-Objekt kapselt den gesamten Zustand einer QUIC-Verbindung, einschließlich der Connection ID, des TLS-Zustands, der Flow-Control-Fenster und der aktiven Streams.
* **Stream-Management**: Es ist die Fabrik für neue Streams. Du verwendest Methoden wie `openBidirectionalStream()` oder High-Level-Wrapper wie `sendRequest()`, um neue Kommunikationskanäle innerhalb der Session zu öffnen.
* **Datenübertragung**: Es bietet Methoden zum Senden und Empfangen von Daten auf spezifischen Streams (`sendHeaders`, `sendBodyChunk`, `streamEvents`).
* **Lebenszyklus-Kontrolle**: Methoden wie `isConnected()` und `close()` ermöglichen die Verwaltung des Lebenszyklus der Verbindung.
* **Herzschlag**: Die `poll()`-Methode auf dem Session-Objekt ist, wie bereits beschrieben, entscheidend, um die Verbindung am Leben zu erhalten und die Netzwerk-State-Machine anzutreiben.

**Beispiel: Interaktion mit einer Server-Session**

Dieses Beispiel zeigt die serverseitige Logik, die ein `Session`-Objekt verwendet, das von `Server->accept()` zurückgegeben wird, um eine einfache HTTP-Anfrage manuell zu bearbeiten.

~~~php
<?php
use Quicpro\Config;
use Quicpro\Server;
use Quicpro\Session;

$config = Config::new([/*...*/]);
$server = new Server('0.0.0.0', 4433, $config);

echo "Manual HTTP server running...\n";

// Die accept()-Methode blockiert, bis eine neue Verbindung ankommt,
// und gibt dann ein Session-Objekt für diesen Client zurück.
while ($session = $server->accept()) {
    // Starte eine Fiber, um diesen Client zu behandeln, damit der
    // Haupt-Thread sofort wieder neue Verbindungen akzeptieren kann.
    (new Fiber(function (Session $clientSession) {
        try {
            echo "Client connected: " . $clientSession->getRemoteAddress() . "\n";
            $requestStreamId = null;

            // Warte auf einen neuen Stream vom Client.
            while ($requestStreamId === null) {
                $clientSession->poll(10);
                $requestStreamId = $clientSession->acceptStream();
            }

            // Lese die Anfrage-Header.
            $headers = $clientSession->receiveHeaders($requestStreamId);
            
            // Sende eine Antwort.
            $responseHeaders = [[':status', '200'], ['content-type', 'text/plain']];
            $clientSession->sendHeaders($requestStreamId, $responseHeaders);
            $clientSession->sendBodyChunk($requestStreamId, "Hello from manual server!", true); // true = FIN
            
            // Schließe die Verbindung, nachdem die Antwort gesendet wurde.
            $clientSession->close();
            echo "Client disconnected.\n";

        } catch (\Exception $e) {
            // Verbindung schließen, wenn ein Fehler auftritt.
            if ($clientSession->isConnected()) $clientSession->close();
        }
    }))->start($session);
}
?>
~~~

### TLS (Transport Layer Security)

**TLS** ist keine direkt sichtbare Komponente, mit der Du ständig interagierst, sondern das fundamentale Sicherheits-Backbone, das in den C-Kern integriert ist. `quicpro_async` erzwingt die Verwendung von **TLS 1.3**, der neuesten und sichersten Version des Protokolls, für alle QUIC-Verbindungen.

**Kernfunktionalitäten:**

* **Verschlüsselung**: Alle über QUIC gesendeten Daten werden standardmäßig mit TLS 1.3 verschlüsselt. Dies ist keine Option, sondern ein integraler Bestandteil des QUIC-Protokolls.
* **Authentifizierung**: Der Kern übernimmt die gesamte Komplexität des TLS-Handshakes, einschließlich der Validierung von Zertifikaten (`verify_peer`) und der Aushandlung von kryptographischen Algorithmen.
* **Mutual TLS (mTLS)**: Das Framework bietet erstklassige Unterstützung für mTLS, bei dem sich sowohl Client als auch Server gegenseitig mit Zertifikaten authentifizieren. Dies wird einfach über die `cert_file` und `key_file` Parameter im `Config`-Objekt konfiguriert und ist die bevorzugte Methode zur Absicherung der internen MCP-Kommunikation.
* **Session Resumption (0-RTT)**: Der TLS-Stack implementiert Session Tickets, die es einem Client ermöglichen, eine frühere Sitzung wieder aufzunehmen und dabei den zeitaufwändigen Handshake zu überspringen. Dies ermöglicht echte 0-RTT-Verbindungen, bei denen Anwendungsdaten bereits im ersten Paket vom Client zum Server gesendet werden können.

**Beispiel: Starten eines Servers mit mTLS-Anforderung**

Dieser Server wird nur Verbindungen von Clients akzeptieren, die ein gültiges, von der internen CA signiertes Zertifikat vorweisen können.

~~~php
<?php
use Quicpro\Config;
use Quicpro\Server;

// Konfiguriere den Server so, dass er Client-Zertifikate anfordert und validiert.
$mtlsServerConfig = Config::new([
    // Das Zertifikat des Servers selbst.
    'cert_file' => './certs/server.pem',
    'key_file'  => './certs/server.key',
    
    // Die Certificate Authority, der wir vertrauen. Nur Clients mit
    // von dieser CA signierten Zertifikaten werden akzeptiert.
    'ca_file'   => './ca/internal_ca.pem',
    
    // Erzwinge die Validierung des Client-Zertifikats.
    'verify_peer' => true, 
]);

$server = new Server('0.0.0.0', 8443, $mtlsServerConfig);
echo "mTLS-only server running. Waiting for authenticated clients...\n";

while ($session = $server->accept()) {
    // Wenn der Code hier ankommt, hat der Client erfolgreich
    // ein gültiges Zertifikat vorgelegt und der Handshake war erfolgreich.
    echo "Authenticated client connected: " . $session->getPeerCertSubject() . "\n";
    // ... Anwendungslogik ...
    $session->close();
}
?>
~~~

### Validation

Die **Validation**-Komponente ist ein interner, aber entscheidender Teil des Frameworks, der für die Robustheit und Sicherheit der Konfiguration sorgt. Immer wenn Du ein `Quicpro\Config`-Objekt erstellst, durchläuft jede einzelne Option eine strenge Validierungsroutine im C-Kern.

**Kernfunktionalitäten:**

* **Typprüfung**: Stellt sicher, dass jeder Konfigurationsparameter den richtigen PHP-Datentyp hat (z.B. `bool`, `long`, `string`, `double`).
* **Bereichsprüfung**: Validiert, dass numerische Werte innerhalb eines erlaubten Bereichs liegen (z.B. müssen Timeouts nicht-negativ sein).
* **Formatprüfung**: Überprüft, ob Strings einem bestimmten Format entsprechen. Dies ist besonders wichtig für sicherheitsrelevante Parameter wie Hostnamen, Dateipfade oder komplexe Strings wie CPU-Affinitäts-Maps.
* **Allowlist-Prüfung**: Stellt sicher, dass der Wert eines Parameters aus einer vordefinierten Liste von erlaubten Werten stammt (z.B. für den `cc_algorithm`).
* **Frühe Fehlererkennung**: Durch die sofortige Validierung bei der Objekterstellung werden Konfigurationsfehler sofort mit einer spezifischen `InvalidConfigValueException` aufgedeckt, anstatt zu unerwartetem Verhalten oder Abstürzen zur Laufzeit zu führen.

**Beispiel: Demonstration der Validierung (führt zu einer Exception)**

Dieses Beispiel zeigt, wie die Validierung einen ungültigen Wert abfängt und einen Fehler wirft.

~~~php
<?php
use Quicpro\Config;
use Quicpro\Exception\InvalidConfigValueException;

echo "Attempting to create a config with an invalid value...\n";

try {
    // Wir übergeben einen negativen Wert an ein Timeout, was nicht erlaubt ist.
    $invalidConfig = Config::new([
        'max_idle_timeout_ms' => -5000, 
    ]);

} catch (InvalidConfigValueException $e) {
    // Die C-Validierungsroutine fängt dies ab und wirft eine spezifische Exception.
    echo "Caught expected exception!\n";
    echo "Message: " . $e->getMessage() . "\n";
    // Die Nachricht wäre z.B.: "Invalid value for 'max_idle_timeout_ms': value must be a non-negative integer."
}
?>
~~~

### WebSocket

Die **WebSocket**-Komponente bietet eine High-Level-Abstraktion für die Erstellung von Echtzeit-Webanwendungen. Sie baut auf dem `Quicpro\Server` und der QUIC/HTTP/3-Schicht auf und implementiert das WebSocket-Protokoll, einschließlich des Upgrade-Handshakes über den HTTP/3 `CONNECT`-Mechanismus.

**Kernfunktionalitäten:**

* **Ereignisgesteuerte API**: Die `Quicpro\WebSocket\Server`-Klasse bietet eine saubere `.on()`-API für die Ereignisse `connect`, `message` und `close`. Dies ist der einfachste und empfohlene Weg, einen WebSocket-Server zu erstellen.
* **Abstraktion des Framings**: Der C-Kern kümmert sich um das komplexe Framing des WebSocket-Protokolls. Du arbeitest in PHP immer mit vollständigen Nachrichten; fragmentierte Nachrichten werden vom Kern automatisch zusammengesetzt.
* **Performance**: Da die Protokoll-Verarbeitung in C erfolgt, können zehntausende von persistenten WebSocket-Verbindungen von einem einzigen, nebenläufigen PHP-Prozess verwaltet werden.
* **Client- und Server-Implementierung**: Das Framework bietet sowohl eine Server- als auch eine Client-Implementierung, was es ermöglicht, komplexe Architekturen zu bauen, in denen PHP-Dienste auch als WebSocket-Clients für andere Dienste agieren.

**Beispiel: Ein einfacher Chat-Server mit der `.on()`-API**

Dieses Beispiel zeigt einen einfachen Chat-Raum. Jede Nachricht, die von einem Client empfangen wird, wird an alle anderen verbundenen Clients weitergeleitet (Broadcast).

~~~php
<?php
use Quicpro\Config;
use Quicpro\WebSocket\Server as WebSocketServer;
use Quicpro\WebSocket\Connection;

$config = Config::new([/*...*/]);
$wsServer = new WebSocketServer('0.0.0.0', 4433, $config);

// Wir verwenden ein einfaches Array, um alle verbundenen Clients zu speichern.
// In einer echten Anwendung würde man hier eine robustere Struktur verwenden.
$clients = new \SplObjectStorage();

$wsServer->on('connect', function (Connection $connection) use ($clients) {
    echo "New client! ({$connection->getId()})\n";
    $clients->attach($connection);
});

$wsServer->on('message', function (Connection $connection, string $message) use ($clients) {
    echo "Message from {$connection->getId()}: {$message}\n";
    // Sende die Nachricht an alle anderen Clients.
    foreach ($clients as $client) {
        if ($client !== $connection) {
            $client->send("Client #{$connection->getId()} says: {$message}");
        }
    }
});

$wsServer->on('close', function (Connection $connection) use ($clients) {
    echo "Client {$connection->getId()} disconnected.\n";
    $clients->detach($connection);
});

echo "Chat server is running...\n";
$wsServer->run();
?>
~~~
