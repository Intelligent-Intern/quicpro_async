<?php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\MCP;
use Quicpro\Server as McpServer;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

const AGENT_HOST = '0.0.0.0';
const AGENT_PORT = 9501;
const MINIO_HOST = 'minio-s3.my-company.com';
const MINIO_PORT = 443;
define('AWS_ACCESS_KEY_ID', getenv('AWS_ACCESS_KEY_ID') ?: 'YOUR_AWS_ACCESS_KEY_ID');
define('AWS_SECRET_ACCESS_KEY', getenv('AWS_SECRET_ACCESS_KEY') ?: 'YOUR_AWS_SECRET_ACCESS_KEY');
define('AWS_S3_REGION', getenv('AWS_S3_REGION') ?: 'eu-central-1');

function get_aws_s3_auth_headers(string $region, string $bucket, string $accessKey, string $secretKey, string $canonicalUri, string $queryString = ''): array
{
    $amzDate = gmdate('Ymd\THis\Z');
    $dateStamp = gmdate('Ymd');
    $service = 's3';
    $host = "{$bucket}.s3.{$region}.amazonaws.com";
    $payloadHash = hash('sha256', '');
    $canonicalHeaders = "host:{$host}\nx-amz-date:{$amzDate}\n";
    $signedHeaders = 'host;x-amz-date';
    $canonicalRequest = "GET\n{$canonicalUri}\n{$queryString}\n{$canonicalHeaders}\n{$signedHeaders}\n{$payloadHash}";
    $algorithm = 'AWS4-HMAC-SHA256';
    $credentialScope = "{$dateStamp}/{$region}/{$service}/aws4_request";
    $stringToSign = "{$algorithm}\n{$amzDate}\n{$credentialScope}\n" . hash('sha256', $canonicalRequest);
    $signingKey = hash_hmac('sha256', 'aws4_request', hash_hmac('sha256', $service, hash_hmac('sha256', $region, hash_hmac('sha256', $dateStamp, "AWS4" . $secretKey, true), true), true), true);
    $signature = hash_hmac('sha256', $stringToSign, $signingKey);
    $authorizationHeader = "{$algorithm} Credential={$accessKey}/{$credentialScope}, SignedHeaders={$signedHeaders}, Signature={$signature}";
    return ['Host' => $host, 'x-amz-date' => $amzDate, 'Authorization' => $authorizationHeader];
}

class MigrationService
{
    private MCP $minioSmallFileClient;
    private MCP $minioLargeFileClient;
    private MCP $minioMassiveFileClient;
    private ?MCP $s3SourceClient = null;

    public function __construct()
    {
        $this->minioSmallFileClient   = new MCP(MINIO_HOST, MINIO_PORT, Config::new(['max_idle_timeout_ms' => 30000]));
        $this->minioLargeFileClient   = new MCP(MINIO_HOST, MINIO_PORT, Config::new(['cc_algorithm' => 'bbr', 'initial_max_data' => 32 * 1024 * 1024]));
        $this->minioMassiveFileClient = new MCP(MINIO_HOST, MINIO_PORT, Config::new(['max_idle_timeout_ms' => 10800000, 'cc_algorithm' => 'bbr']));
    }

    private function getS3ClientForBucket(string $bucketName): MCP
    {
        if ($this->s3SourceClient === null || $this->s3SourceClient->getTargetHost() !== "{$bucketName}.s3." . AWS_S3_REGION . ".amazonaws.com") {
            $s3Host = "{$bucketName}.s3." . AWS_S3_REGION . ".amazonaws.com";
            $this->s3SourceClient = new MCP($s3Host, 443, Config::new(['verify_peer' => true, 'alpn' => ['h3']]));
        }
        return $this->s3SourceClient;
    }

    /**
     * @throws Exception
     */
    public function listSourceFiles(string $requestPayload): iterable
    {
        $request = IIBIN::decode('ListRequest', $requestPayload);
        $sourceBucket = parse_url($request['source_path'], PHP_URL_HOST);
        $region = AWS_S3_REGION;
        $continuationToken = null;

        if (empty($sourceBucket)) {
            throw new \InvalidArgumentException("Could not parse bucket from source_path: " . $request['source_path']);
        }

        $s3Client = $this->getS3ClientForBucket($sourceBucket);

        if (AWS_ACCESS_KEY_ID === 'YOUR_AWS_ACCESS_KEY_ID') {
            throw new \RuntimeException("AWS credentials are not configured.");
        }

        do {
            $queryString = 'list-type=2&max-keys=1000';
            if ($continuationToken) {
                $queryString .= '&continuation-token=' . urlencode($continuationToken);
            }
            $path = '/?' . $queryString;

            $headers = get_aws_s3_auth_headers($region, $sourceBucket, AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, '/', $queryString);
            $xmlResponse = $s3Client->request('S3', 'GET', $path, $headers);

            if (empty($xmlResponse)) { break; }

            $xml = new \SimpleXMLElement($xmlResponse);
            foreach ($xml->Contents as $content) {
                yield IIBIN::encode('FileMetadata', [
                    'path' => "s3://{$sourceBucket}/" . (string)$content->Key,
                    'size' => (int)$content->Size,
                ]);
            }
            $continuationToken = isset($xml->NextContinuationToken) ? (string)$xml->NextContinuationToken : null;
        } while ($continuationToken);
    }

    public function uploadSmallFile(string $requestPayload): string
    {
        return $this->executeUpload($requestPayload, $this->minioSmallFileClient, 'migrated-thumbnails');
    }

    public function uploadLargeFile(string $requestPayload): string
    {
        return $this->executeUpload($requestPayload, $this->minioLargeFileClient, 'migrated-videos');
    }

    public function uploadMassiveFile(string $requestPayload): string
    {
        return $this->executeUpload($requestPayload, $this->minioMassiveFileClient, 'migrated-archives');
    }

    private function executeUpload(string $requestPayload, MCP $minioClient, string $targetBucket): string
    {
        $request = IIBIN::decode('FileUploadRequest', $requestPayload);
        $sourcePath = $request['source_path'];
        $sourceBucket = parse_url($sourcePath, PHP_URL_HOST);
        $sourceKey = ltrim(parse_url($sourcePath, PHP_URL_PATH), '/');
        $sourceUri = '/' . $sourceKey;

        $s3Client = $this->getS3ClientForBucket($sourceBucket);

        $getHeaders = get_aws_s3_auth_headers(AWS_S3_REGION, $sourceBucket, AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, $sourceUri, '');
        $fileContent = $s3Client->request('S3', 'GET', $sourceUri, $getHeaders);

        if (empty($fileContent)) {
            return IIBIN::encode('UploadResponse', ['status' => 'FAILED_DOWNLOAD']);
        }

        $putPayload = IIBIN::encode('MinioPutObjectRequest', ['bucket' => $targetBucket, 'key' => $sourceKey, 'body' => $fileContent]);
        $putResponsePayload = $minioClient->request('MinioBridge', 'PutObject', $putPayload);
        $putResponse = IIBIN::decode('MinioPutObjectResponse', $putResponsePayload);

        return IIBIN::encode('UploadResponse', ['status' => $putResponse['status']]);
    }
}

function define_required_schemas(): void
{
    $schemas = [
        'ListRequest'           => ['source_path' => ['type' => 'string', 'tag' => 1]],
        'FileMetadata'          => ['path' => ['type' => 'string', 'tag' => 1], 'size' => ['type' => 'int64', 'tag' => 2]],
        'FileUploadRequest'     => ['source_path' => ['type' => 'string', 'tag' => 1]],
        'UploadResponse'        => ['status' => ['type' => 'string', 'tag' => 1]],
        'MinioPutObjectRequest' => ['bucket' => ['type' => 'string', 'tag' => 1], 'key' => ['type' => 'string', 'tag' => 2], 'body' => ['type' => 'bytes', 'tag' => 3]],
        'MinioPutObjectResponse'=> ['status' => ['type' => 'string', 'tag' => 1], 'error' => ['type' => 'string', 'tag' => 2, 'optional' => true]],
    ];

    foreach ($schemas as $name => $definition) {
        if (!IIBIN::defineSchema($name, $definition)) {
            throw new \RuntimeException("Fatal: Failed to define required IIBIN schema '{$name}'.");
        }
    }
}

try {
    define_required_schemas();

    $serverConfig = Config::new([
        'cert_file' => './certs/agent_cert.pem',
        'key_file'  => './certs/agent_key.pem',
    ]);

    $server = new McpServer(AGENT_HOST, AGENT_PORT, $serverConfig);

    $migrationService = new MigrationService();
    $server->registerService('MigrationService', $migrationService);

    $server->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal agent error occurred: " . $e->getMessage() . "\n");
}