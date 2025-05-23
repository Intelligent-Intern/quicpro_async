# Fiber & Async in PHP 8.1–8.3: How to Use `quicpro_async` in Userland

## Summary

- PHP 8.4 supports native Fiber suspend/resume inside extensions – **no manual work needed.**
- On PHP 8.1–8.3 you *must* handle Fiber switching in your own code for real async/non-blocking I/O.
- This file gives you code patterns and explains what breaks if you ignore it.

---

## What is a Fiber?

A **Fiber** in PHP is like a lightweight “pause-and-resume” for part of your code.  
It lets you write async code in a synchronous style.  
But in PHP 8.1–8.3, built-in extensions (like quicpro_async) **cannot** suspend execution automatically from C.  
That means *your script* must control when to yield (pause) and resume.

---

## Why does this matter?

- On PHP 8.4+, `quicpro_async` does the right thing: blocking calls yield the current Fiber (e.g. `poll()` pauses if it would block, resumes when ready).
- On PHP 8.1–8.3, if you call `Session::poll()` or similar from the main thread (not inside a Fiber), it just blocks your whole script. No real concurrency.
- **If you want async/concurrent I/O in 8.1–8.3, you have to run blocking parts inside your own Fibers and manage yielding yourself.**

---

## Example: Manual Fiber Scheduler for PHP 8.3

Here’s a minimal pattern for PHP 8.3 userland async:

~~~php
use Quicpro\Config;
use Quicpro\Session;

$cfg  = Config::new();
$sess = new Session('cloudflare-quic.com', 443, $cfg);

$fiber = new Fiber(function() use ($sess) {
    $id = $sess->sendRequest('/');
    while (!$resp = $sess->receiveResponse($id)) {
        Fiber::suspend();      // you must yield control manually
    }
    print $resp[":status"]."\n".$resp['body'];
});

// Cooperative event loop: resume Fiber, then poll network, then repeat
while (!$fiber->isTerminated()) {
    $fiber->resume();
    $sess->poll(10);           // poll for I/O; this may block if not careful
    // you can add timers, check multiple sessions, etc.
}
~~~

**You must never call poll() or blocking network APIs from outside a Fiber**  
unless you want your whole PHP process to block.

---

## When Should I Upgrade to PHP 8.4?

- Native Fiber support in extensions is only reliable in PHP 8.4+.
- If you use PHP 8.4 or newer, the extension yields automatically – you don’t have to touch Fibers, just write normal code.
- On 8.3 and below, you *must* wrap blocking calls in Fibers and yield/suspend explicitly.

---

## Gotchas / Limitations

- Forgetting to yield/suspend means your code blocks like oldschool PHP.
- Errors inside Fibers may be lost if you don’t handle them.
- Manual scheduling only works if you know all I/O points in advance.
- If you want real async across multiple HTTP/3/WebSocket sessions, you need a scheduler loop (see examples in the repo).

---

## TL;DR

- PHP 8.4: Just write normal async code.
- PHP 8.1–8.3: Use Fibers, yield/suspend explicitly, or you get blocking.

---

## See also

- [PHP Manual: Fibers](https://www.php.net/manual/en/class.fiber.php)
- [quicpro_async README](README.md)

For questions or real-world scheduler patterns, see `examples/` or open an issue.
