<?php
declare(strict_types=1);

/*
 * Stub class for situations where the C extension has not yet defined it.
 * Only loaded in test/dev context via Composer "autoload-dev".
 */
if (!class_exists('QuicServerException', false)) {
    class QuicServerException extends RuntimeException {}
}
