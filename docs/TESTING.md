# TESTING.md
Everything you need to verify, fuzz and benchmark **QuicPro Async** – on
your laptop *or* in the cloud.

---

## 1 · Quick start table

| Target            | What it does                            | Typical runtime |
|-------------------|-----------------------------------------|-----------------|
| `make unit`       | Runs all PHPUnit suites (php 8.1-8.4)   | 5-30 s          |
| `make fuzz`       | Launches AFL/LibFuzzer on the C layer   | open-ended      |
| `make benchmark`  | Spins Hetzner CX-servers, fires RPS test| 2-8 min         |

---

## 2 · Prerequisites

* PHP 8.1+ with **phpunit** (`composer global require phpunit/phpunit`)
* GCC / Clang, `make`, `pkg-config`
* AFL++ or LibFuzzer in `PATH` (for fuzz)
* Pulumi v4 CLI (`brew install pulumi`) **and** a Hetzner Cloud token in
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

## 8 · Contribute new tests

1. **PHPUnit** – drop the file under `tests/phpunit/<area>/XyzTest.php`.
2. **Fuzz** – add a C harness in `tests/fuzz/`, register in `Makefile`.
3. **Benchmarks** – extend `benchmarks/pulumi/run.sh` with a new mode.

All new files are auto-picked by `make unit` and CI.  
Happy testing! 🚀
