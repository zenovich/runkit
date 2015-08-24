--TEST--
removing magic __tostring method
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class Test {
    function __tostring() {echo '__tostring';}
}

$a = new Test();
(string) $a;
runkit_method_remove("Test", "__tostring");
(string) $a;
?>
--EXPECTF--
__tostring
Catchable fatal error: Method Test::__toString() must return a string value in %s on line %d
