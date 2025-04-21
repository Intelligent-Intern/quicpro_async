# quicpro_async 🚀
Native QUIC / HTTP‑3 for PHP 8.x – non‑blocking & Fiber‑ready

[![PECL](https://img.shields.io/badge/PECL-quicpro__async-blue?logo=php)](https://intelligentinternpecl.z13.web.core.windows.net/packages/quicpro_async)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## Why should I care? 🌱⚡️

HTTP‑3 (QUIC + TLS 1.3) removes at least one round‑trip per new connection
and multiplexes all requests on **one UDP socket**.  
For busy PHP workloads this yields:

* **up to 80‑90 % less CPU time for series requests** –  
  in our benchmark 1 000 requests: 0.008 s (QUIC) vs 0.054 s (cURL)
* **shorter TTFB**, especially on mobile / lossy networks
* **fewer file descriptors / epoll entries** – pack more tenants per host

`quicpro_async` delivers these benefits directly in PHP – no external
curl binaries, no Go sidecars, no reverse proxies.

---

## Highlights

* **Full HTTP‑3 client** – connect, send, receive, close
* **Zero‑RTT** and session tickets out‑of‑the‑box
* **Non‑blocking API** – drive progress with `quicpro_poll()`
* **Fiber‑aware** – automatic yield / `await`‑style coding
* Stats (`rtt`, `loss`, `cwnd`) & QLOG tracing for observability
* Builds on Linux / macOS / Windows, quiche included as git‑submodule
* MIT‑licensed & published on our PECL channel

---

## Quick install (PECL)

~~~bash
pecl channel-discover https://intelligentinternpecl.z13.web.core.windows.net/channel.xml
pecl install quicpro_async
echo "extension=quicpro_async.so" \
     > $(php -i | grep -Po '/.*?/conf\.d')/30-quicpro_async.ini
~~~

For manual builds or distro packages see **BUILD.md**.

---

## 3‑liner example

~~~php
<?php
$sess = quicpro_connect('cloudflare-quic.com', 443);
$id   = quicpro_send_request($sess, '/');

while (!$resp = quicpro_receive_response($sess, $id)) {
    quicpro_poll($sess, 50);  // yields inside a Fiber
}

echo $resp['body'];
quicpro_close($sess);
~~~

More use cases (Fibers, object wrapper, pipelines) in **EXAMPLES.md**.

---

## Who benefits?

| Persona              | Impact                                                                      |
|----------------------|------------------------------------------------------------------------------|
| **Site owners**      | Faster first byte, better Core Web Vitals, greener footprint.                |
| **Hosting providers**| Fewer sockets & CPU → higher tenant density, “HTTP‑3 ready” badge.           |
| **PHP developers**   | Real async HTTP client with plain PHP – no curl‑multi, no GDNS hacks.        |
| **Serverless users** | Lower cold‑start cost: 0‑RTT + one socket that survives container reuse.     |

---

## Under the hood 🔍

* Cloudflare **quiche** (≥ 0.22) handles transport, congestion, TLS 1.3
* Lightweight C shim binds quiche FFI to Zend API
* `quicpro_poll()` → `select()` → `quiche_conn_recv/send` → Fiber yield
* Single resource struct stores socket, transport & H3 connection
* Build: `phpize && ./configure --enable-quicpro_async` – cargo builds quiche

---

## Roadmap

* **0.2** – server mode (`quicpro_listen`), server push
* **1.0** – stable API, PSR‑7 adapter, Prometheus metrics
* **1.1** – WebTransport & DATAGRAM frames
* **1.2** – automatic congestion tuning for LTE / 5G

---

## Maintainer

Jochen Schultz – [jschultz@php.net](mailto:jschultz@php.net)  
Feedback, bug reports and PRs are highly welcome!  
See **CONTRIBUTING.md** for guidelines.

---

## License

Distributed under the **MIT License**.  
See the full text in **LICENSE**.

Looking forward to seeing what you build with HTTP‑3‑native PHP 🚀