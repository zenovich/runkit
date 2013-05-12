--TEST--
runkit_default_property_add() function with overriding objects
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class RunkitClass {
}

$className = 'RunkitClass';
$propName = 'publicProperty';
$value = 1;
$obj = new $className();
runkit_default_property_add($className, 'constArray', array('a'=>1), RUNKIT_OVERRIDE_OBJECTS);
runkit_default_property_add($className, $propName, $value, RUNKIT_ACC_PUBLIC | RUNKIT_OVERRIDE_OBJECTS);
runkit_default_property_add($className, 'privateProperty', "a", RUNKIT_ACC_PRIVATE | RUNKIT_OVERRIDE_OBJECTS);
runkit_default_property_add($className, 'protectedProperty', NULL, RUNKIT_ACC_PROTECTED | RUNKIT_OVERRIDE_OBJECTS);
runkit_default_property_add($className, 'dynamic', $obj, RUNKIT_OVERRIDE_OBJECTS);
$value = 10;
print_r($obj);

$obj = new stdClass();
runkit_default_property_add('stdClass', 'str', 'test', RUNKIT_OVERRIDE_OBJECTS);
print_r($obj);
?>
--EXPECTF--
RunkitClass Object
(
    [constArray] => Array
        (
            [a] => 1
        )

    [publicProperty] => 1
    [privateProperty%sprivate] => a
    [protectedProperty:protected] =>%w
%w[dynamic] => RunkitClass Object
%w*RECURSION*%w
)

Warning: runkit_default_property_add(): Adding properties to internal classes is not allowed in %s on line %d
stdClass Object
(
)
