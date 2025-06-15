## A Guide to Authentication & Authorization with `quicpro_async`

### 1. The Modern Authentication & Authorization Challenge

In any complex, real-world system, authentication and authorization are not a feature; they are the foundation upon which trust and security are built. A modern platform must securely manage the identities of and enforce access policies for a wide variety of "principals":

* **Human Users:** Employees like dispatchers and managers who log in via a web interface.
* **End Customers:** External users who need limited access, for example, to track a single delivery.
* **Devices (IoT):** Automated clients, like the thousands of vehicles in our fleet, which must authenticate securely without human interaction.
* **Backend Services:** The microservices themselves, which need to securely prove their identity when communicating with each other.

The challenge is to create a unified system that can handle these diverse authentication mechanisms while providing a flexible, fine-grained authorization model. A monolithic approach, where each service implements its own auth logic, is insecure, difficult to maintain, and leads to inconsistencies.

### 2. Our Architectural Philosophy: A Decoupled Identity Broker

Our architecture treats identity and access management as a first-class, specialized concern. Instead of scattering auth logic across the system, we centralize it within a dedicated, decoupled microservice: the **`Auth Agent`**. This agent acts as the central **Identity Broker** for the entire platform.

This design is supported by a second service, the **`Data Retrieval Agent`**. This clean separation of concerns is critical:

* **The `Auth Agent` (The Brain):** This is the public-facing component of our auth system. It is responsible for handling all authentication **protocols and logic**. It knows how to process a password, how to execute an OAuth 2.0 flow with a third party like GitHub, and how to validate a device's secret key. Its sole purpose is to verify a principal's identity and, upon success, issue a standardized internal access token (a JWT). It is stateless and knows nothing about how or where user and device credentials are stored.

* **The `Data Retrieval Agent` (The Vault):** This is a simple, internal MCP service that acts as an abstraction layer in front of our credential database. It exposes a generic API, such as `findPrincipalByIdentifier(string $identifier)`, which the `Auth Agent` calls. This design is extremely flexible; for development and testing, it can be backed by a simple **SQLite** file for a zero-configuration setup. In production, it can be configured to connect to a highly-available **PostgreSQL** cluster, all without changing a single line of code in the `Auth Agent`.

### 3. A Hybrid Authentication Strategy for a Zero-Trust Environment

To secure our platform, we employ a hybrid strategy that provides security at both the transport and application layers, creating a zero-trust environment where no communication is trusted by default.

* **For Service-to-Service Communication (East-West Traffic): Mutual TLS (mTLS)**
  All internal communication between our backend agents (e.g., Ingestion Gateway calling the Persistence Agent) is secured using strict mTLS. Each service is issued a unique TLS certificate from a private, internal Certificate Authority (CA). A connection between two services is only established if both can present a valid certificate signed by this trusted CA. This is configured directly in the `Quicpro\Config` object and enforced at the C-level by the `quicpro_async` engine, providing the highest level of transport security and ensuring that no unauthorized service can communicate on our internal network.

* **For End-Clients (North-South Traffic): JSON Web Tokens (JWTs)**
  Managing TLS certificates for millions of users and devices is impractical. For these external principals, we use a stateless, token-based system. A client authenticates **once** against our `Auth Agent` (using their password, an OAuth flow, or a device secret) and receives a short-lived **JSON Web Token (JWT)**. This JWT is a signed, self-contained credential that the client presents with every subsequent request.

  Our resource servers (like the Ingestion Gateway or the Live Operations Hub) can then **locally and instantly** verify the JWT's signature using the `Auth Agent`'s public key. This method is incredibly fast as it requires no network call back to the `Auth Agent`, allowing us to validate thousands of requests per second with negligible performance impact.

### 4. Fine-Grained Authorization with Scopes

Authentication (proving who you are) is only half the story. Authorization (defining what you are allowed to do) is what enforces business rules. We handle this using **scopes** embedded directly within our internal JWTs.

When the `Auth Agent` issues a token, it includes a `scope` claim that lists the specific permissions for that principal, for example: `scope: "telemetry:write"` for a vehicle, or `scope: "map:view route:read"` for a dispatcher. Each microservice is then responsible for inspecting this scope and verifying that the authenticated principal has the necessary permission before executing an action. This provides granular, decentralized access control throughout the entire system.

The following sections will provide detailed, runnable examples demonstrating these concepts in practice, including a deep dive into implementing a secure mTLS-protected service and a sophisticated GitHub OAuth 2.0 flow that performs authorization based on repository access.

## Part 2: Service-to-Service Auth with Mutual TLS (mTLS)

### 1. The Concept: Enforcing a Zero-Trust Backend Network

In a distributed microservices architecture, securing communication *between* services (often called "East-West" traffic) is as critical as securing communication from external clients. We operate on a "Zero-Trust" principle: no service should inherently trust a network request from another, even if it originates from within the same datacenter.

To achieve this, we use **Mutual TLS (mTLS)**. Unlike standard TLS where only the client verifies the server's identity, mTLS enforces a two-way cryptographic verification:

1.  The client verifies that the server is who it claims to be by checking its certificate.
2.  The server, in turn, verifies that the client is who it claims to be by checking the client's certificate.

A connection is only established if **both** parties can present a valid certificate signed by a trusted Certificate Authority (CA). For our internal architecture, we use a private, self-hosted CA. This means that only services that have been explicitly issued a certificate from our internal CA can communicate with each other, effectively creating a secure, encrypted, and fully authenticated service mesh.

### 2. The Prerequisite: Creating a Private CA & Service Certificates

To make this example fully runnable, you must first generate the necessary cryptographic materials. The following steps use the standard `openssl` command-line tool to create a simple, self-signed Certificate Authority and issue unique certificates for a "Publisher" and a "Subscriber" service.

**Step 1: Create the Internal Certificate Authority (CA)**

This CA is the root of trust for our internal services. First, create a directory structure.
~~~bash
mkdir -p ca/ certs/
~~~

Next, generate the CA's private key and its public certificate.
~~~bash
# Create the private key for the CA
openssl genrsa -out ./ca/ca.key 4096

# Create the self-signed root certificate for the CA
openssl req -new -x509 -days 3650 -key ./ca/ca.key -out ./ca/ca.pem \
  -subj "/C=DE/ST=NRW/L=Meckenheim/O=Quicpro Internal/CN=Quicpro Internal CA"
~~~

**Step 2: Create Keys and Certificate Signing Requests (CSRs) for Each Service**

Each service needs its own unique keypair.

~~~bash
# Create a private key for the Publisher service
openssl genrsa -out ./certs/publisher.key 2048

# Create a CSR for the Publisher service
openssl req -new -key ./certs/publisher.key -out ./certs/publisher.csr \
  -subj "/C=DE/ST=NRW/L=Meckenheim/O=Quicpro Internal/CN=publisher.internal"

# Create a private key for the Subscriber service
openssl genrsa -out ./certs/subscriber.key 2048

# Create a CSR for the Subscriber service
openssl req -new -key ./certs/subscriber.key -out ./certs/subscriber.csr \
  -subj "/C=DE/ST=NRW/L=Meckenheim/O=Quicpro Internal/CN=subscriber.internal"
~~~

**Step 3: Sign the Service Certificates with the CA**

Use the CA key to sign the CSRs, creating the final, trusted certificates for each service.

~~~bash
# Sign the Publisher's certificate
openssl x509 -req -days 365 -in ./certs/publisher.csr \
  -CA ./ca/ca.pem -CAkey ./ca/ca.key -CAcreateserial \
  -out ./certs/publisher.pem

# Sign the Subscriber's certificate
openssl x509 -req -days 365 -in ./certs/subscriber.csr \
  -CA ./ca/ca.pem -CAkey ./ca/ca.key -CAcreateserial \
  -out ./certs/subscriber.pem
~~~

You now have all the necessary files in the `./ca/` and `./certs/` directories for the following PHP examples to run.

### 3. Mermaid Diagram: The mTLS Handshake Flow

This diagram illustrates the two-way verification process that occurs when a Publisher client connects to a Subscriber server using mTLS.

~~~mermaid
graph LR
    subgraph "Trust Anchor"
        CA("fa:fa-file-signature Internal CA <br> - ");
    end

    subgraph "Service A: Publisher (Client Role)"
        Publisher["fa:fa-paper-plane Publisher Agent<br/>(MCP Client)<br> - "];
    end

    subgraph "Service B: Subscriber (Server Role)"
        Subscriber["fa:fa-satellite-dish Subscriber Agent<br/>(MCP Server)<br> - "];
    end
    
    CA -- "Issues Cert A" --> Publisher;
    CA -- "Issues Cert B" --> Subscriber;

    Publisher -- "Initiates Connection" --> Subscriber;
    Subscriber -- "Presents Cert B" --> Publisher;
    Publisher -- "Verifies Cert B against CA" --> CA;
    
    Publisher -- "Presents Cert A" --> Subscriber;
    Subscriber -- "Verifies Cert A against CA" --> CA;
    
    Subscriber -- "Handshake OK" --> Publisher;
    
    Publisher -- "mTLS Secured MCP Connection" <--> Subscriber;
~~~

### 4. Implementation: A Simple, mTLS-Secured Pub/Sub System

The key to the implementation is how we configure the `Quicpro\Config` object for both the client and the server, pointing them to their respective certificates and the shared CA.

#### The Subscriber (`subscriber.php`)

This script starts an MCP server that will only accept connections from clients presenting a valid, trusted certificate.

~~~php
<?php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) { die("Quicpro extension not loaded.\n"); }

use Quicpro\Config;
use Quicpro\Server as McpServer;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

try {
    echo "Starting mTLS-secured Subscriber Server...\n";

    $serverConfig = Config::new([
        'verify_peer' => true, // Force the server to demand and verify a client certificate.
        'alpn'        => ['mtls-pubsub/1.0'],
        'ca_file'     => './ca/ca.pem', // Path to the trusted internal CA certificate.
        'cert_file'   => './certs/subscriber.pem', // The server's own certificate.
        'key_file'    => './certs/subscriber.key', // The server's private key.
    ]);
    
    IIBIN::defineSchema('SimpleMessage', ['message' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('Ack', ['status' => ['type' => 'string', 'tag' => 1]]);

    $server = new McpServer('127.0.0.1', 9999, $serverConfig);
    
    $handler = new class {
        public function receiveMessage(string $payload): string {
            $message = IIBIN::decode('SimpleMessage', $payload);
            echo "SUCCESS: Received authenticated message: '{$message['message']}'\n";
            return IIBIN::encode('Ack', ['status' => 'RECEIVED']);
        }
    };

    $server->registerService('PubSubService', $handler);
    echo "Listening on udp://127.0.0.1:9999. Awaiting authenticated publishers...\n";
    $server->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal server error occurred: " . $e->getMessage() . "\n");
}
~~~

#### The Publisher (`publisher.php`)

This script acts as a client that must present its own certificate to connect to the subscriber.

~~~php
<?php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) { die("Quicpro extension not loaded.\n"); }

use Quicpro\Config;
use Quicpro\MCP;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

try {
    echo "Starting mTLS-secured Publisher Client...\n";

    $clientConfig = Config::new([
        'verify_peer' => true, // Standard server verification.
        'alpn'        => ['mtls-pubsub/1.0'],
        'ca_file'     => './ca/ca.pem', // Path to the same trusted internal CA.
        'cert_file'   => './certs/publisher.pem', // The CLIENT's own certificate to present to the server.
        'key_file'    => './certs/publisher.key', // The CLIENT's private key.
    ]);

    IIBIN::defineSchema('SimpleMessage', ['message' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('Ack', ['status' => ['type' => 'string', 'tag' => 1]]);

    echo "Connecting to Subscriber at udp://127.0.0.1:9999...\n";
    $subscriberClient = new MCP('127.0.0.1', 9999, $clientConfig);
    
    echo "Connection established. Publishing message...\n";
    $payload = IIBIN::encode('SimpleMessage', ['message' => 'TOP_SECRET_DATA_PACKAGE_ALPHA']);
    
    $responsePayload = $subscriberClient->request('PubSubService', 'receiveMessage', $payload);
    $response = IIBIN::decode('Ack', $responsePayload);
    
    echo "SUCCESS: Server acknowledged receipt with status: '{$response['status']}'\n";

} catch (QuicproException | \Exception $e) {
    // If the publisher's certificate is invalid, or the server's, this will throw a TlsHandshakeException.
    die("A fatal client error occurred: " . $e->getMessage() . "\n");
}
~~~

### 5. The Convenience Layer: The Optional Composer Package

While the `quicpro_async` PECL extension provides the high-performance engine, it is designed to be lean and focused on core networking primitives. For higher-level, application-specific logic, a companion Composer package, `intelligent-intern/quicpro_async`, provides a rich set of pre-built components and convenience utilities.

This package includes a production-ready implementation of the `Auth Agent` and `Data Retrieval Agent` architecture, complete with out-of-the-box support for various OAuth 2.0 providers (e.g., GitHub, Microsoft) and other common authentication patterns.

For a developer who wants to quickly add a feature like "Login with GitHub" to their application, the workflow is drastically simplified:

1.  **Install the Engine:** `pecl install quicpro_async`
2.  **Get the Application Logic:** `composer require intelligent-intern/quicpro_async`
3.  **Configure and Deploy:** The package includes a `Makefile` with targets like `make auth-setup`. Running this command interactively configures the system by asking which providers to enable, generates the necessary keys and configuration stubs, and deploys the agents.

This dual-component approach offers the best of both worlds: the raw power and performance of a C extension, combined with the rapid development, flexibility, and rich feature set of a high-level PHP library. A developer can have a production-ready, high-performance authentication system running in minutes, not weeks.

## Part 3: Client Auth with OAuth 2.0 & Fine-Grained Authorization

### 1. The Concept: Delegated Authentication with Post-Verification

For many modern applications, especially those tailored for developers or technical teams, allowing users to sign in with their existing GitHub account is a crucial feature. This is achieved using the **OAuth 2.0** protocol, which allows a user to delegate authentication to a trusted third party without sharing their password.

However, simple authentication (proving a user is who they say they are) is often not enough. A professional system requires **fine-grained authorization** (checking what a user is allowed to do). In this example, we will implement a "Login with GitHub" flow that goes a step further: after authenticating the user, our system will make an additional API call to GitHub to verify that the user has at least read access to a specific, required private repository. A user will only be granted access to our platform if they are both a valid GitHub user and a collaborator on that repository.

### 2. The Architectural Pattern: The All-Knowing `Auth Agent`

This entire complex workflow, including all secure communication with the GitHub API, is encapsulated within our `Auth Agent`. The rest of our microservices ecosystem remains completely unaware that GitHub is involved. They continue to operate exclusively with the internal JWTs issued by our agent. This pattern provides three key benefits:

* **Security:** All sensitive interactions, including the exchange of the OAuth `client_secret` and `access_token`, are confined to a single, secure backend service. The frontend never touches these credentials.
* **Abstraction:** The rest of our system is shielded from the complexity of different authentication providers. The process for handling a user authenticated via GitHub is identical to handling one authenticated via a password.
* **Flexibility:** The `Auth Agent` becomes the central identity broker. Adding another provider, like GitLab or Microsoft, simply means adding a new method to this agent, with no changes required elsewhere.

### 3. Mermaid Diagram: The GitHub OAuth Flow with Authorization Check

This diagram illustrates the complete, multi-step sequence, from the initial user click to the final issuance of our internal JWT after a successful repository access check.

~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef frontend fill:#ef6c00,color:white
    classDef auth_agent fill:#c2185b,color:white,stroke-width:2px
    classDef external fill:#424242,color:white
    classDef internal_service fill:#512da8,color:white

    subgraph "User & Frontend"
        A(fa:fa-user User) -- 1. Clicks 'Login with GitHub' --> B[Application Frontend];
    end

    subgraph "External Provider"
        C("fa:fa-github GitHub Auth Screen<br> - ");
        D("fa:fa-github-alt GitHub API<br> - ");
    end

    subgraph "Our Backend (PHP + Quicpro)"
        E["fa:fa-user-secret Auth Agent<br/>(MCP Server)<br> - "];
        F["fa:fa-database Data Retrieval Agent<br/>(MCP Server)<br> - "];
    end

    %% --- OAuth Flow ---
    B -- Redirects to --> C;
    C -- User authorizes app --> G{Redirect to<br/>Callback URL<br> - };
    G -- "With temporary 'code'" --> B;
    B -- "MCP Call:<br/>loginWithProvider('github', code)<br> - " --> E;
    
    E -- "Exchanges 'code' for 'access_token'<br/>(Server-to-Server HTTPS)<br> - " --> D;
    D -- "Returns 'access_token'" --> E;
    
    E -- "Authorization Check<br/>API Call: 'Is user a collaborator<br/>on private-repo?'<br> - " --> D;
    D -- "Returns '204 No Content' (Yes) or '404 Not Found' (No)" --> E;
    
    E -- "If authorized, find/create user<br/>(MCP Call)<br> - " --> F;
    F -- "Returns internal user data" --> E;
    
    E -- "Issues internal JWT" --> B;
    B -- "Session Established" --> A;

    %% --- Apply Styles ---
    class A,B frontend;
    class C,D external;
    class E auth_agent;
    class F internal_service;
~~~

### 4. Implementation: Extending the `Auth Agent`

To implement this, we add a new `handleGithubLogin` method to our `AuthAgentService` class. This method contains the complete logic for the OAuth flow and the subsequent authorization check.

~~~php
<?php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\MCP;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;
use Quicpro\Exception\PolicyViolationException;

// --- Application & OAuth Configuration ---
// In a real application, these would be loaded from a secure environment file or service.
const GITHUB_CLIENT_ID = getenv('GITHUB_CLIENT_ID') ?: 'your_github_client_id';
const GITHUB_CLIENT_SECRET = getenv('GITHUB_CLIENT_SECRET') ?: 'your_github_client_secret';
const REQUIRED_GITHUB_REPO = 'my-organization/our-private-project';

// --- JWT Signing Key ---
// The private key for signing our internal JWTs MUST be loaded securely from the environment.
// It should NEVER be hardcoded.
const JWT_PRIVATE_KEY_PATH = getenv('JWT_SIGNING_KEY_PATH');

class AuthAgentService
{
    private MCP $dataRetrievalClient;
    private MCP $githubApiClient;
    private string $jwtPrivateKey;

    public function __construct()
    {
        if (empty(JWT_PRIVATE_KEY_PATH) || !file_exists(JWT_PRIVATE_KEY_PATH)) {
            throw new \RuntimeException(
                "JWT signing key not found. Please set the 'JWT_SIGNING_KEY_PATH' environment variable.\n" .
                "You can generate a new key with: openssl ecparam -name prime256v1 -genkey -noout -out /path/to/private.pem"
            );
        }
        $this->jwtPrivateKey = file_get_contents(JWT_PRIVATE_KEY_PATH);

        $internalConfig = Config::new(['alpn' => ['mcp/1.0']]);
        $this->dataRetrievalClient = new MCP('dataretriever.internal', 9900, $internalConfig);
        
        $externalConfig = Config::new(['verify_peer' => true, 'alpn' => ['h3', 'h2']]);
        $this->githubApiClient = new MCP('api.github.com', 443, $externalConfig);
    }

    public function login(string $provider, array $credentials): string
    {
        $handlerMethod = 'handle' . ucfirst(strtolower($provider)) . 'Login';

        if (method_exists($this, $handlerMethod)) {
            return $this->{$handlerMethod}($credentials);
        }

        throw new \InvalidArgumentException("No handler available for authentication provider '{$provider}'.");
    }

    private function handleGithubLogin(array $credentials): string
    {
        if (empty($credentials['code'])) {
            throw new \InvalidArgumentException("GitHub OAuth flow requires a 'code'.");
        }

        $tokenResponse = $this->githubApiClient->request(
            'HTTPS', 'POST', '/login/oauth/access_token',
            ['Accept' => 'application/json', 'User-Agent' => 'QuicproPlatform'],
            json_encode([
                'client_id' => GITHUB_CLIENT_ID,
                'client_secret' => GITHUB_CLIENT_SECRET,
                'code' => $credentials['code'],
            ])
        );

        $tokenData = json_decode($tokenResponse['body'], true);
        if (!isset($tokenData['access_token'])) {
            throw new QuicproException("Failed to get access token from GitHub: " . ($tokenData['error_description'] ?? 'Unknown error'));
        }
        $accessToken = $tokenData['access_token'];

        $authHeaders = ['Authorization' => "Bearer {$accessToken}", 'User-Agent' => 'QuicproPlatform'];
        $userDataResponse = $this->githubApiClient->request('HTTPS', 'GET', '/user', $authHeaders);
        $githubUser = json_decode($userDataResponse['body'], true);
        $githubLogin = $githubUser['login'];
        $githubEmail = $githubUser['email'];

        $repoCheckPath = "/repos/" . REQUIRED_GITHUB_REPO . "/collaborators/" . $githubLogin;
        $authzResponse = $this->githubApiClient->request('HTTPS', 'GET', $repoCheckPath, $authHeaders);
        
        if ($authzResponse['status'] !== 204) {
            throw new QuicproException("Authorization failed: User '{$githubLogin}' does not have access to required repository '" . REQUIRED_GITHUB_REPO . "'. Status: " . $authzResponse['status']);
        }

        $principalPayload = $this->dataRetrievalClient->request(
            'DataService',
            'findOrCreatePrincipalByIdentifier',
            IIBIN::encode('FindRequest', ['identifier' => $githubEmail])
        );
        $principal = IIBIN::decode('PrincipalData', $principalPayload);

        return $this->issueInternalJwt($principal['internal_id'], ['map:view', 'route:read', 'repo_access:' . REQUIRED_GITHUB_REPO]);
    }
    
    private function issueInternalJwt(string $userId, array $scopes): string
    {
        $header = ['alg' => 'ES256', 'typ' => 'JWT'];
        $payload = [
            'iat' => time(),
            'exp' => time() + 3600, // 1 hour expiration
            'sub' => $userId,
            'scope' => implode(' ', $scopes),
            'iss' => 'quicpro_auth_agent',
        ];

        $base64UrlHeader = str_replace(['+', '/', '='], ['-', '_', ''], base64_encode(json_encode($header)));
        $base64UrlPayload = str_replace(['+', '/', '='], ['-', '_', ''], base64_encode(json_encode($payload)));

        $signature = '';
        $success = openssl_sign(
            $base64UrlHeader . "." . $base64UrlPayload,
            $signature,
            $this->jwtPrivateKey,
            OPENSSL_ALGO_SHA256
        );

        if (!$success) {
            throw new \RuntimeException("Failed to sign JWT using the provided key.");
        }

        $base64UrlSignature = str_replace(['+', '/', '='], ['-', '_', ''], base64_encode($signature));

        return $base64UrlHeader . "." . $base64UrlPayload . "." . $base64UrlSignature;
    }
}
~~~