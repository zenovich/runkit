--TEST--
removing magic unserialize method
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class Test implements Serializable {
	function serialize() {return "";}
	function unserialize($s) {}
}

$a = new Test();
runkit_method_remove("Test", "unserialize");
$s1 = serialize($a);
unserialize($s1);
?>
--EXPECTF--
Fatal error: Couldn't find implementation for method Test::unserialize in Unknown on line %d
