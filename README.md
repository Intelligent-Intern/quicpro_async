# THIS IS STILL WORK IN PROGRESS - GIVE ME A COUPLE DAYS TO FINISH IT.. DOCUMENTATION FIRST YAY

## debugged the extension - next step: adding more tests and writing a shell script that enables them (using demo docker files basically and run unittests inside 4 containers 8.1 - 8.4 to test all versions in parallel)

# quicpro_async
Native QUIC / HTTP-3 and WebSocket support for PHP 8.1-8.4 – non-blocking, Fiber-first

[![PECL](https://img.shields.io/badge/PECL-quicpro__async-blue?logo=php)](https://intelligentinternpecl.z13.web.core.windows.net/packages/quicpro_async)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## Why care about HTTP-3 & QUIC?

HTTP-3 collapses the TCP+TLS handshake into a single UDP roundtrip and multiplexes all requests over a single secure connection. In PHP workloads this means:

* -80% CPU for keep-alive request bursts (1k HTTPS benchmark vs. curl TCP)
* significantly lower TTFB on lossy / mobile networks
* one socket per origin – fewer FD limits, smaller epoll sets

quicpro_async brings this directly into PHP – no shell-outs to curl, no Go or Rust sidecars, no reverse proxy layer.

---

## Feature highlights

| Area                  | Status                                          |
|-----------------------|-------------------------------------------------|
| HTTP-3 client         | connect, 0-RTT, header & body streaming         |
| WebSocket client/server (RFC 9220) | upgrade, frame send/recv, Fiber-ready      |
| QUIC server mode      | SO_REUSEPORT multi-worker, TLS ticket cache     |
| Observability         | stats struct (rtt/loss/cwnd), perf-ring, QLOG   |
| XDP / Busy-poll path  | optional --enable-quicpro-xdp build flag        |
| Cross-platform        | Linux glibc/musl, macOS (darwin-arm64), WSL     |

---

## Installation (PECL)

~~~bash
pecl channel-discover https://intelligentinternpecl.z13.web.core.windows.net/channel.xml
pecl install quicpro_async
echo "extension=quicpro_async.so" > $(php -i | grep -Po '/.*?/conf\.d')/30-quicpro_async.ini
~~~
Manual build? See BUILD.md.

---

## TL;DR – 5-line GET using the OOP API

~~~php
use Quicpro\Config;
use Quicpro\Session;

$cfg  = Config::new();
$sess = new Session('cloudflare-quic.com', 443, $cfg);
$id   = $sess->sendRequest('/');
while (!$resp = $sess->receiveResponse($id)) $sess->poll(25);
print $resp[":status"]."\n".$resp['body'];
~~~

More demos (streaming uploads, WebSocket hub, TLS-resume workers) in examples/.

---

## Who benefits?

|                   | Impact                                         |
|-------------------|------------------------------------------------|
| Developers        | First async HTTP client that's pure PHP         |
| Ops / Hosts       | Fewer sockets, higher tenant density           |
| Edge & FaaS       | 0-RTT handshakes, one socket survives warm-ups |

---

## How it works

* quiche >= 0.23 handles transport, congestion, TLS 1.3
* C shim exposes quiche FFI to Zend VM; heavy crypto stays in Rust
* Fiber-aware poll loop – Session::poll() uses busy-poll/XDP or falls back to select()
* Shared-memory ticket LRU enables resumption across forked workers

---

## Async and Fiber Support (PHP 8.1–8.4)

quicpro_async is built around async operations and PHP’s Fiber model. What does that mean for you?

- A Fiber in PHP is a way to run multiple things in parallel *without blocking the rest of your script*. Think of it as a really lightweight cooperative thread.
- Starting with PHP 8.1, basic Fiber support is present, but native integration (especially for things like suspend/resume inside internal functions) is only solid in PHP 8.4.
- On PHP <8.4, you only get real async if you implement your own Fiber management in userland (see `php8.3.fiber.md`). This means *you* have to schedule and yield Fibers around blocking calls yourself if you want concurrent I/O.
- On PHP 8.4+, the extension handles Fiber scheduling internally: if your script is running inside a Fiber, blocking network calls will yield automatically. No hacks, no manual yield, just drop-in async.

If you want to write portable code across PHP 8.1–8.4, check the userland Fiber helper patterns and caveats in [`php8.3.fiber.md`](php8.3.fiber.md).

If you want *zero boilerplate* async HTTP/3 and WebSocket in PHP, just use PHP 8.4+.

---

## Roadmap

* 0.2 – Priority frames (RFC 9218), path-MTU probing
* 0.3 – Retry token box & DoS hardening
* 1.0 – Stable PSR-7 adapter, Prometheus metrics, Windows native build

---

## Contributing

PRs, issues and ideas welcome. Please read CONTRIBUTING.md. Each PR is fuzz-checked and run against the full CI matrix (Ubuntu & Alpine, PHP 8.1–8.4).

Maintainer – Jochen Schultz · <jschultz@php.net>

---

### License
MIT; see LICENSE.

Happy hacking.
