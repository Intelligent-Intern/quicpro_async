# quicpro_async – AI Agent Examples

---

## 1 ·  Streaming chat completion (Server-Sent Events)

**Why?**  
Stream tokens from an LLM back-end (e.g. OpenAI, Ollama, vLLM) to the
browser without waiting for the full answer.

~~~php
$sess   = quicpro_connect('llm.internal', 4433);
$body   = json_encode([
    'model'  => 'gpt-4o',
    'stream' => true,
    'messages' => [['role'=>'user','content'=>'Hello, who won the 2022 World Cup?']]
]);
$hdrs   = ['content-type'=>'application/json'];

$id = quicpro_send_request($sess, '/v1/chat/completions', $hdrs, $body);

while (!$r = quicpro_receive_response($sess, $id)) {
    quicpro_poll($sess, 10);                     // max 10 ms latency
}
foreach (explode("\n", $r['body']) as $line) {
    if (str_starts_with($line, 'data: ')) {
        echo substr($line, 6);                   // stream token to stdout
    }
}
quicpro_close($sess);
~~~

---

## 2 ·  Ten parallel prompts with Fibers

**Why?**  
Bulk-score a list of product reviews, extract keywords, or translate many
sentences in a single PHP worker without blocking.

~~~php
function ask(string $prompt): string {
    $s  = quicpro_connect('llm.internal', 4433);
    $id = quicpro_send_request($s, '/v1/completions',
        ['content-type'=>'application/json'],
        json_encode(['model'=>'gpt-4o','prompt'=>$prompt])
    );
    while (!$r = quicpro_receive_response($s, $id)) quicpro_poll($s, 5);
    quicpro_close($s);
    return json_decode($r['body'], true)['choices'][0]['text'];
}

$prompts = array_map(fn($i)=>"Summarise text $i", range(1,10));
$fibers  = array_map(fn($p)=>new Fiber(fn() => ask($p)), $prompts);
foreach ($fibers as $f) $f->start();
foreach ($fibers as $idx=>$f) echo "#$idx → ", $f->getReturn(), PHP_EOL;
~~~

---

## 3 ·  Bidirectional WebSocket to a local RAG pipeline

**Why?**  
Keep a long-lived connection to a Retrieval-Augmented-Generation server
so each query reuses the same context cache and avoids TLS overhead.

~~~php
$sess = quicpro_connect('rag.local', 8443);
$ws   = $sess->upgrade('/chat');          // RFC 9220 CONNECT → WS

fwrite($ws, json_encode(['ask'=>'What is E-MC²?'])."\n");

while (!feof($ws)) {
    $msg = fgets($ws);
    echo "Agent says: ", $msg;
    if (str_contains($msg, '[DONE]')) break;
}
fclose($ws);
$sess->close();
~~~

---

## 4 ·  Fork-per-core agent gateway (zero-RTT + Fibers)

**Why?**  
Expose many AI models behind one UDP port; each worker process handles its
own epoll loop while the shared ticket ring preserves 0-RTT reconnects.

~~~php
use Quicpro\Config;
use Quicpro\Server;
use Fiber;

$cores = (int) trim(shell_exec('nproc')) ?: 4;
$cfg   = Config::new([
    'cert_file'=>'cert.pem', 'key_file'=>'key.pem',
    'alpn'=>['h3'], 'session_cache'=>true
]);

for ($i = 0; $i < $cores; $i++) {
    if (pcntl_fork() === 0) { serve($i, 4433, $cfg); exit; }
}
pcntl_wait($s);

function serve(int $wid, int $port, Config $cfg): void {
    $srv = new Server('0.0.0.0', $port, $cfg);
    echo "[worker $wid] online\n";
    while ($s = $srv->accept(50)) (new Fiber(fn() => route($s)))->start();
}

function route($sess): void {
    $req  = $sess->receiveRequest();
    $path = $req->headers()[':path'] ?? '/';
    match ($path) {
        '/v1/chat/completions' => proxy_to('llm1.internal', $sess, $req),
        '/v1/embeddings'       => proxy_to('embed.internal', $sess, $req),
        default                => $sess->close()
    };
}

function proxy_to(string $backend, $sess, $req): void {
    /* left as exercise: send to backend, stream back to client */
    $sess->close();
}
~~~

## 5 ·  Invoking a Model Context Protocol (MCP) tool over HTTP/3

**Why?**  
MCP is the new open standard (adopted by Anthropic *and* OpenAI) that lets
any LLM call external **tools** through one uniform interface.  
When your PHP app can hit an MCP server you gain **model-agnostic** tool
invocation: Claude, GPT-4o, or an open-source model can all run the same
calculator, database, or vector-search function without custom glue.

### Scenario
*You run a remote MCP server that exposes a `calculator` tool via HTTP
Server-Sent Events (SSE). The goal is to ask the tool “sqrt(144)” and
stream the result back as soon as it is ready.*

~~~php
use Fiber;

/* 1. open one QUIC session to the MCP server */
$sess = quicpro_connect('mcp.example', 4433);

/* 2. MCP servers use SSE for streaming responses             */
/*    POST body = JSON with tool name + arguments             */
$body = json_encode([
    'tool'   => 'calculator',
    'input'  => ['expression' => 'sqrt(144)'],
    'stream' => true                   // ask for streaming
]);
$hdrs = [
    'content-type' => 'application/json',
    'accept'       => 'text/event-stream',
];

/* 3. send the request */
$stream = quicpro_send_request($sess, '/mcp/v1/call_tool', $hdrs, $body);

/* 4. read Server-Sent Events as they come in                 */
/*    every event line starts with  “data: {JSON…}”           */
while (true) {
    $resp = quicpro_receive_response($sess, $stream);
    if ($resp) break;                  // final chunk received
    quicpro_poll($sess, 5);            // yield ≤ 5 ms
}

/* 5. split the SSE payload into lines, show only “data: …”   */
foreach (explode("\n", $resp['body']) as $line) {
    if (str_starts_with($line, 'data: ')) {
        $event = json_decode(substr($line, 6), true);
        printf("Δ %s\n", $event['chunk'] ?? $event['result'] ?? '');
    }
}

quicpro_close($sess);
~~~

*Output*

~~~
Δ 1 Δ 2 Δ . Δ 0
~~~


The tool streamed four chunks: “1”, “2”, “.”, “0”. Concatenated they give
**12.0** – the square root of 144.

### Error handling cheat-sheet

Most failures map to the standard `ERROR_AND_EXCEPTION_REFERENCE.md`.

| Situation                           | Likely exception / error code           | Quick fix                              |
|-------------------------------------|-----------------------------------------|----------------------------------------|
| Tool name typo                      | `QuicErrorUnknownFrame` / 4xx JSON      | Check `tool` field spelling            |
| MCP server unreachable              | `QuicSocketException`                   | Verify DNS / firewall                  |
| Bad JSON in request                 | `QuicTransportException` (HTTP 400)     | `json_encode`-safe and re-send         |
| SSE disconnect mid-stream           | `QuicStreamException`                   | Retry idempotent call or fall back     |

---

These five examples now cover single-shot requests, parallel Fibers,
stream uploads, WebSockets, fork-per-core gateways **and** the new MCP
tool standard – everything you need to wire PHP agents to modern LLM
stacks.
