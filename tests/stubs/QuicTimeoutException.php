<?php
declare(strict_types=1);

/*
 * Stub class for timeout-related errors. Loaded only when the C
 * extension hasn’t registered its native definition yet.
 */
if (!class_exists('QuicTimeoutException', false)) {
    class QuicTimeoutException extends RuntimeException {}
}
