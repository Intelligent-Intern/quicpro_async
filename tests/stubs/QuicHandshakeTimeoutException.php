<?php
declare(strict_types=1);

/*
 * Stub to satisfy static analysis before the extension is loaded.
 */
if (!class_exists('QuicHandshakeTimeoutException', false)) {
    class QuicHandshakeTimeoutException extends RuntimeException {}
}
