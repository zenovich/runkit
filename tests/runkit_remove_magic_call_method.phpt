--TEST--
removing magic __call method
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class Test {
    function __call($m, $args) {echo '__call';}
}

$a = new Test();
$a->method();
runkit_method_remove("Test", "__call");
$a->method();
?>
--EXPECTF--
__call
Fatal error: Call to undefined method Test::method() in %s on line %d
