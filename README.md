# THIS IS STILL WORK IN PROGRESS - GIVE ME A COUPLE DAYS TO FINISH IT.. DOCUMENTATION FIRST YAY


# quicpro_async 🚀
Native QUIC / HTTP‑3 **and WebSocket** support for PHP 8.1‑8.4 – non‑blocking, Fiber‑first

[![PECL](https://img.shields.io/badge/PECL-quicpro__async-blue?logo=php)](https://intelligentinternpecl.z13.web.core.windows.net/packages/quicpro_async)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## Why care about HTTP‑3 & QUIC? 🌱⚡️

HTTP‑3 collapses the TCP+TLS handshake into **one UDP flight** and multiplexes all requests over a single secure connection.  In PHP workloads this brings

* **‑80 % CPU** for keep‑alive request bursts ¹
* **significantly lower TTFB** on lossy / mobile networks
* **one socket per origin** – fewer FD limits, smaller epoll sets

`quicpro_async` delivers these wins **inside PHP itself** – no curl shell‑outs, no Go sidecars, no reverse proxy layer.

---

## Feature highlights

| Area | Status |
|------|--------|
| HTTP‑3 **client** | ✔ connect • 0‑RTT • header & body streaming |
| WebSocket **client & server** (RFC 9220) | ✔ upgrade • frame send/recv • Fiber‑friendly |
| QUIC **server mode** | ✔ SO_REUSEPORT multi‑worker • TLS ticket cache |
| Observability | ✔ stats struct (rtt/loss/cwnd) • perf‑ring • optional QLOG/UDP |
| **XDP / Busy‑poll** path | optional `--enable-quicpro-xdp` build flag |
| Cross‑platform | Linux glibc / musl, macOS (darwin‑arm64), Windows WSL |

> ¹ benchmark: 1 000 HTTPS requests – nginx 1.25 (QUIC) vs curl‑TLS+TCP on same host.

---

## Installation (PECL)

```bash
pecl channel-discover https://intelligentinternpecl.z13.web.core.windows.net/channel.xml
pecl install quicpro_async
# enable extension
echo "extension=quicpro_async.so" > $(php -i | grep -Po '/.*?/conf\.d')/30-quicpro_async.ini
```
Manual build? See **BUILD.md**.

---

## TL;DR – 5‑line GET using the OOP API

```php
use Quicpro\Config;
use Quicpro\Session;

$cfg  = Config::new();                    // defaults: verify_peer=on, ALPN h3
$sess = new Session('cloudflare-quic.com', 443, $cfg);
$id   = $sess->sendRequest('/');
while (!$resp = $sess->receiveResponse($id)) $sess->poll(25);
print $resp[":status"]."\n".$resp['body'];
```
More demos – streaming uploads, WebSocket broadcast hub, TLS‑resume workers – in **examples/**.

---

## Who benefits?

| | Impact |
|---|---|
| **Developers** | First async HTTP client that is *pure PHP* – no ext/curl juggling |
| **Ops / Hosts** | Fewer sockets → higher tenant density • QUIC by default badge |
| **Edge & FaaS** | 0‑RTT handshakes + one socket survive warm starts |

---

## How it works 🔍

* **quiche** ≥ 0.23 does transport, congestion, TLS 1.3.
* Thin **C shim** exposes quiche FFI to the Zend VM; all heavy crypto stays in Rust.
* **Fiber‑aware poll loop** – `Session::poll()` integrates busy‑poll/XDP or falls back to `select()`.
* **Shared‑memory ticket LRU** enables resumption across forked workers.

---

## Roadmap

* **0.2** – Priority frames (RFC 9218), path‑MTU probing
* **0.3** – Retry token box & DoS hardening
* **1.0** – Stable PSR‑7 adapter, Prometheus metrics, Windows native build

---

## Contributing

PRs, issues and ideas welcome! Please read **CONTRIBUTING.md**.  Each PR is fuzz‑checked and run against the full CI matrix (Ubuntu & Alpine, PHP 8.1‑8.4).

Maintainer – Jochen Schultz · <jschultz@php.net>

---

### License
MIT; see LICENSE.

*Happy hacking — bring your PHP apps to HTTP‑3 speed!* 🚀

