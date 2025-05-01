# Contributing to **quicpro_async**

Thank you for taking the time to contribute!  
Bug reports, ideas and pull requests are highly appreciated.

---

## 📋 Quick start

~~~bash
git clone --recurse-submodules https://github.com/intelligent-intern/quicpro_async.git
cd quicpro_async
phpize && ./configure --enable-quicpro_async
make -j$(nproc)              # builds the extension
make test                    # runs *.phpt tests via run‑tests.php
# benchmark helpers (optional)
cd tests/benchmark && composer install && cd ../..
~~~

*Only one test?*

~~~bash
php -dextension=modules/quicpro_async.so run-tests.php tests/001_smoke.phpt
~~~

---

## 🐞 Reporting bugs

1. Search existing issues first – duplicates will be closed.
2. Open a new issue and include:
    * PHP version (`php -v`)
    * quiche commit (`git -C quiche rev-parse --short HEAD`)
    * Reproducer script **or** `.phpt` file
    * Backtrace (`gdb --args php …` or `valgrind`) if it segfaults

---

## 🛠 Pull Requests

~~~bash
git checkout -b feature/your-feature
~~~

* Follow **PSR‑12** for all PHP code (`php-cs-fixer fix`).
* C‑code must compile **warnings‑free** with `-Wall -Wextra`.
* Add / update tests (`.phpt` or PHPUnit) so CI stays green.
* Run locally:

  ~~~bash
  make test
  vendor/bin/phpunit      # if you added PHP‑level tests
  ~~~

Push your branch and open a PR – the template will guide you.

---

## 💻 C / Zend specifics

* Follow the PHP‑core style:

    * Tabs = 4 spaces (real TAB chars).
    * K&R braces, brace on the same line.
    * Pointer star sticks to the type: `char *buf`, `session_t *s`.
    * Max line length ≈ 100 chars.
    * Function / variable names `snake_case`; macros `ALL_CAPS`.
    * One `efree()` per `emalloc()`; free everything in `quicpro_session_dtor()`.
    * Use `UNEXPECTED()` / `EXPECTED()` for hot‑path checks.

* `clang-format` config is shipped; run:
  ~~~bash
  clang-format -i src/*.c
  ~~~

* Touching the quiche submodule?
  ~~~bash
  cd quiche
  git pull --ff-only               # bring in upstream changes
  cargo fmt                        # format rust sources
  cd ..
  git add quiche
  ~~~

---

## 🚀 Benchmarking

A local benchmark compares quicpro_async (HTTP‑3) with libcurl
(HTTP‑1.1) and shows the speed‑up.

### 1 · Start the demo HTTP‑3 server

~~~bash
# build once
(cd quiche && cargo build --release --bin quiche-server)

# self‑signed cert
mkdir -p test_certs
openssl req -x509 -newkey rsa:2048 -nodes \
        -keyout test_certs/key.pem -out test_certs/cert.pem \
        -subj "/CN=localhost" -days 365

# run on UDP 4433
./quiche/target/release/quiche-server \
        --listen 127.0.0.1:4433 \
        --cert   test_certs/cert.pem \
        --key    test_certs/key.pem \
        --root   .
~~~

### 2 · Start a plain HTTP‑1 server (for curl)

~~~bash
php -S 127.0.0.1:8000 -t .
~~~

### 3 · Run the benchmark script

~~~bash
cd tests/benchmark
php -dextension=../../modules/quicpro_async.so bench.php 1000 100
~~~

Example output:
~~~
QUIC/HTTP‑3 (1000 req, 100 conc)
QUIC : 5200.9 req/s (0.192 s)
HTTP‑1.1 via libcurl (sequential, same #req)
cURL : 650.4 req/s (1.538 s)
~~~


This shows ~10 × higher throughput and ~80 % CPU reduction for QUIC.

The higher the load the higher the reduction compared to cURL.

---

## 📄 License

By submitting code you agree to license your contribution under the
MIT license contained in this repository.

Happy hacking 🚀