--TEST--
runkit_method_remove() function with ReflectionParameter
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
class RunkitClass {
	function runkitMethod(RunkitClass $param) {
		echo "Runkit Method\n";
	}
}

$obj = new RunkitClass();

$reflMethod = new ReflectionMethod('RunkitClass', 'runkitMethod');
$reflParam = $reflMethod->getParameters();
$reflParam = $reflParam[0];

runkit_method_remove('RunkitClass','runkitMethod');

var_dump($reflParam);
var_dump($reflParam->getDeclaringFunction());
?>
--EXPECTF--
object(ReflectionParameter)#%d (1) {
  ["name"]=>
  string(31) "__parameter_removed_by_runkit__"
}

Fatal error:%sInternal error: Failed to retrieve the reflection object in %s on line %d
