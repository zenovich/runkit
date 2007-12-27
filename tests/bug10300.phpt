--TEST--
Bug #10300 Segfault when copying __call()
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class Mixin
{
    public function __call($message, $arguments)
    {
    }
}

class Test
{
}

runkit_method_copy('Test', '__call', 'Mixin');

$t = new Test;
$t->test();
?>
--EXPECT--

