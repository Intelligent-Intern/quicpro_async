<?php
declare(strict_types=1);

/*
 *  WHY A STUB?
 *  -----------
 *  The real `QuicConnectionException` class is constructed inside the C
 *  extension at runtime.  Static code analysers and PHPUnit’s bootstrap
 *  phase execute *before* the extension is loaded, so we shim a minimal
 *  definition **only when it does not exist yet**.  The stub is loaded
 *  through Composer’s `autoload-dev` section and never ships in prod.
 */
if (!class_exists('QuicConnectionException', /* autoload = */ false)) {
    class QuicConnectionException extends RuntimeException {}
}
