--TEST--
runkit_default_property_add() function (PHP4)
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) != 4) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=On
--FILE--
<?php
class RunkitClass {
}

ini_set('display_errors', 1);
ini_set('error_reporting', E_ALL);

$className = 'RunkitClass';
$propName = 'publicProperty';
runkit_default_property_add($className, 'constArray', array('a'=>1));
runkit_default_property_add($className, $propName, 1, RUNKIT_ACC_PUBLIC);
runkit_default_property_add($className, 'privateProperty', "a", RUNKIT_ACC_PRIVATE);
runkit_default_property_add($className, 'protectedProperty', NULL, RUNKIT_ACC_PROTECTED);
$obj = new $className();
runkit_default_property_add($className, 'dynamic', $obj);
print_r(new $className());

runkit_default_property_add('stdClass', 'str', 'test');

$obj = new stdClass();
print_r($obj);
?>
--EXPECTF--
runkitclass Object
(
    [constArray] => Array
        (
            [a] => 1
        )

    [publicProperty] => 1
    [privateProperty] => a
    [protectedProperty] =>%w
    [dynamic] => runkitclass Object
        (
            [constArray] => Array
                (
                    [a] => 1
                )

            [publicProperty] => 1
            [privateProperty] => a
            [protectedProperty] =>%w
        )

)

Warning: runkit_default_property_add(): Adding properties to internal classes is not allowed in %s on line %d
stdClass Object
(
)
