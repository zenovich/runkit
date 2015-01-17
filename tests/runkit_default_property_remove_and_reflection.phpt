--TEST--
runkit_method_remove() function with reflection
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
class RunkitClass {
	public $publicProperty;
}

$obj = new RunkitClass();

$reflClass = new ReflectionClass('RunkitClass');
$reflObject = new ReflectionObject($obj);
$reflProp = new ReflectionProperty('RunkitClass', 'publicProperty');

runkit_default_property_remove('RunkitClass','publicProperty', TRUE);

try {
	var_dump($reflClass->getProperty('publicProperty'));
	echo 'No exception!';
} catch (ReflectionException $e) {
}

try {
	var_dump($reflObject->getProperty('publicProperty'));
	echo 'No exception!';
} catch (ReflectionException $e) {
}
var_dump($reflProp);
$reflProp->setValue($obj, 'test');
?>
--EXPECTF--
object(ReflectionProperty)#%d (2) {
  ["name"]=>
  string(30) "__property_removed_by_runkit__"
  ["class"]=>
  string(11) "RunkitClass"
}

Fatal error:%sInternal error: Failed to retrieve the reflection object in %s on line %d
