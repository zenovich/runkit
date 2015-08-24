--TEST--
removing magic __callstatic method
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
      if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--FILE--
<?php
class Test {
    static function __callstatic($m, $args) {echo '__callstatic';}
}

Test::method();
runkit_method_remove("Test", "__callstatic");
Test::method();
?>
--EXPECTF--
__callstatic
Fatal error: Call to undefined method Test::method() in %s on line %d
