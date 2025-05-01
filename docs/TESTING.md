# TESTING.md
Everything you need to verify, fuzz and benchmark **QuicPro Async** – on
your laptop *or* in the cloud.

---

## 1 · Quick start table

| Target            | What it does                            | Typical runtime  |
|-------------------|-----------------------------------------|------------------|
| `make unit`       | Runs all PHPUnit suites (php 8.1-8.4)   | 5-30 s           |
| `make fuzz`       | Launches AFL/LibFuzzer on the C layer   | open-ended @TODO |
| `make benchmark`  | Spins Hetzner CX-servers, fires RPS test| 2-8 min @TODO    |

---

## 2 · Prerequisites

* PHP 8.1+ with **phpunit** (`composer global require phpunit/phpunit`)
* GCC / Clang, `make`, `pkg-config`
* AFL++ or LibFuzzer in `PATH` (for fuzz)
* Pulumi v4 CLI (`brew/apt/chocolatey install pulumi`) **and** a Hetzner Cloud token in
  `HETZNER_TOKEN` (for benchmark)

---

## 3 · Unit tests

### 3.1 Run all versions locally

~~~bash
make unit              # runs php 8.1, 8.2, 8.3, 8.4 in infra/containers
~~~

### 3.2 Run a single suite

~~~bash
./infra/scripts/run-tests-php83.sh tests/phpunit/connection
~~~

### 3.3 What is covered

* Connection, stream and WebSocket happy paths
* All documented Exceptions (see ERROR_AND_EXCEPTION_REFERENCE.md)
* Time-out, retry and edge-case scenarios
* Smoke-test 001 loads the PECL and checks `quicpro_version()`

---

## 4 · Fuzz tests

### 4.1 Launch continuous AFL

~~~bash
make fuzz              # builds hongfuzz / afl++ harness
~~~

*Harness location* `tests/fuzz/*.c`  
Seed corpus lives in `tests/fuzz/corpus/`.  
Coverage reports appear in `tests/fuzz/out/`.

Stop with `Ctrl-C`; findings are written as `crash-*` files.

---

## 5 · Benchmarks (Pulumi + Hetzner)

### 5.1 First run

~~~bash
export HETZNER_TOKEN=your-token-here      # scope: read+write projects
make benchmark
~~~

The script will:

1. Create a temporary Pulumi stack.
2. Spawn **2× CX21** servers in the same vSwitch (1 server + 1 client).
3. Install `quicpro_async`, wrk2 and TLS certs via cloud-init.
4. Run `benchmarks/pulumi/run.sh` (100 k parallel requests, 2 MiB body).
5. Dump `benchmarks/results/*.json` and destroy the stack.

Estimated cost: **< €0.10** per full run.

### 5.2 Custom parameters

~~~bash
make benchmark RPS=5000 DURATION=120 REGION=nbg1
~~~

---

## 6 · CI integration

* **GitHub Actions**: `.github/workflows/ci.yml` executes `make unit`
  and `make fuzz` (15 min hard limit).
* `act` users can dry-run with `make deploy-test` (requires the
  [nektos/act] binary).

---

## 7 · Troubleshooting

| Symptom                         | Hint                                          |
|---------------------------------|-----------------------------------------------|
| “No rule to make … php_quicpro.c” | Run `phpize && ./configure` again            |
| PHPUnit fails on php 8.4 only   | Check `ext-version` conditional in stub file  |
| Fuzzer reports OOM              | Export `AFL_NO_AFFINITY=1`, lower `MEM_LIMIT` |
| Pulumi stack not deleted        | `pulumi stack rm -f bench-<timestamp>`        |

---
# Test descriptions

### 002-connection / ClientConnectTest

| ID | What it asserts | Spec reference | Why it matters |
|----|-----------------|----------------|----------------|
| 2-1 | `quicpro_connect()` returns a **connected** `Quicpro\Session` object | RFC 9000 §6.4-6.5, RFC 9114 §4.1 | Every higher-level feature depends on a clean QUIC + HTTP/3 handshake. |
| 2-2 | `quicpro_get_stats()` exposes `rtt` **and** `version` keys | RFC 9000 §18 | Applications need transport metrics for congestion tuning and VN diagnostics. |
| 2-3 | Unknown host raises **`QuicConnectionException`** | RFC 9000 §21.5 | Explicit errors allow caller retries and prevent silent fall-through to TCP. |

### Fixture / environment

* **demo-quic** (Docker) – static IP `172.19.0.10:4433/udp`, self-signed TLS
* Env vars  
  `QUIC_DEMO_HOST` (default `demo-quic`)  
  `QUIC_DEMO_PORT` (default `4433`)
* Extension must be loaded; otherwise the suite is skipped.

> Passing this test guarantees that the project’s QUIC handshake layer is
> functional on all supported PHP versions (8.1 … 8.4). Every subsequent
> suite (TLS, streams, resumption) can rely on that baseline.


---

### 002-connection / HandshakeTest

| ID | Assertion | Spec reference | Why we care |
|----|-----------|----------------|-------------|
| 2-4 | ALPN **h3** is negotiated (`quicpro_get_alpn`) | RFC 9114 §4.1 | Guarantees HTTP/3 semantics; prevents silent draft fallback |
| 2-5 | Client succeeds after a **Retry** packet (`retry = true`) | RFC 9000 §8 | Required when servers use Retry for anti-DoS |
| 2-6 | No application data sent before handshake confirmation | RFC 9000 §4.1 | Protects against replay; compliant with 0-RTT rules |

*Fixture update*: `demo-quic` now exposes **port 4434/udp** that always
issues a Retry packet so test 2-5 has a deterministic trigger.

---

### 002-connection / ServerAcceptTest

| ID | Assertion | Spec reference | Why we care |
|----|-----------|----------------|-------------|
| 2-7 | `Server->accept()` yields a **Session** within 5 s | RFC 9000 §6 | Proves the listener can parse Client Initial and complete handshake |
| 2-8 | HTTP/3 request echoed with status **200** | RFC 9114 §4 | Confirms STREAM creation & HEADERS encoding on both ends |
| 2-9 | Child process exits with **code 0** | N/A | Detects hidden segfaults or blocking I/O inside the C wrapper |

**Runner impact**  
This test spins up a server **inside the PHPUnit process**, so the
docker-compose matrix and `run-tests-matrix.sh` **require no changes**.  
Each PHP container binds an ephemeral high port (> 50 000) that never
conflicts with other tests, hence parallel runs stay safe.

---

### 002-connection / StreamOpenTest

| ID | Assertion | Spec reference | Why we care |
|----|-----------|----------------|-------------|
| 2-10 | First client-initiated **bidi** stream ID = 0 | RFC 9000 §2.1 | Confirms stream-type encoding and starting point |
| 2-11 | Second bidi stream ID = +4 | RFC 9000 §2.1 | Detects off-by-one that would clash with uni streams |
| 2-12 | First **uni** stream ID = 2 | RFC 9000 §2.1 | Verifies unidirectional type flag & ID calculation |

**Runner impact**  
All tests run inside the existing PHP containers and use the same
`demo-quic` service.  No change to `run-tests-matrix.sh` or the compose
file is required.  Parallel execution is safe because stream IDs are
local to each session.

---

### 002-connection / StreamTransferTest

| ID | Assertion | Spec reference | Why we care |
|----|-----------|----------------|-------------|
| 2-13 | 5-byte body echoed intact | RFC 9114 §6 | Baseline DATA frame integrity |
| 2-14 | 1 MiB body echoed intact, correct size | RFC 9000 §4 | Validates flow-control & reassembly |
| 2-15 | FIN flag respected; client still reads post-FIN | RFC 9000 §3.2 | Ensures duplex semantics & END_STREAM handling |

**Runner impact**  
Uses existing `demo-quic` `/echo` endpoint.  No change to
`run-tests-matrix.sh` or compose file is needed.  The large-payload test
may add ~50 ms runtime per PHP version—well within current CI budget.

---

### 002-connection / WebsocketOpenTest

| ID | Assertion | Spec reference | Why we care |
|----|-----------|----------------|-------------|
| 2-16 | `/chat` upgrade returns **stream resource** | RFC 9220 §3 | Confirms CONNECT handshake and ws-over-h3 mapping |
| 2-17 | Echoed payload equals sent payload | RFC 6455 §5, RFC 9220 §6 | Validates bidirectional data flow over QUIC STREAM |
| 2-18 | Half-closing write side keeps read open | RFC 6455 §5.5 + QUIC FIN | Needed for request–reply patterns (e.g. SSE-style chat) |

**Runner impact**

* The existing `demo-quic` image already contains the echo-chat handler
  (`/srv/chat_server.php`), so **no new docker service** or shell-script
  change is required.

---


## XXXXXXXXXXXXXXX · Contribute new tests

1. **PHPUnit** – drop the file under `tests/phpunit/<area>/XyzTest.php`.
2. **Fuzz** – add a C harness in `tests/fuzz/`, register in `Makefile`.
3. **Benchmarks** – extend `benchmarks/pulumi/run.sh` with a new mode.

All new files are auto-picked by `make unit` and CI.  
Happy testing! 🚀
