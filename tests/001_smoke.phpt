--TEST--
quicpro_async: extension loads and exposes core functions
--SKIPIF--
<?php
if (!extension_loaded('quicpro_async')) {
    echo 'skip quicpro_async not loaded';
}
?>
--FILE--
<?php
echo function_exists('quicpro_connect') ? "OK\n" : "missing\n";
?>
--EXPECT--
OK