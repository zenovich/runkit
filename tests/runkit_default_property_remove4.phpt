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
    var $constArray = array();
    var $publicProperty = 1;
    var $publicproperty = 2;
}

ini_set('display_errors', 1);
ini_set('error_reporting', E_ALL);

$className = 'RunkitClass';
$obj = new $className();
runkit_default_property_add($className, 'dynamic', $obj);

runkit_default_property_remove($className, 'dynamic');
runkit_default_property_remove($className, 'publicproperty');
runkit_default_property_remove($className, 'publicproperty');
runkit_default_property_remove($className, 'constArray');
print_r(new $className());

$obj = new stdClass();
runkit_default_property_remove('stdClass', 'str');
?>
--EXPECTF--
Warning: runkit_default_property_remove(): RunkitClass::publicproperty does not exist in /home/dzenovich/runkit-my/tests/runkit_default_property_remove4.php on line %d
runkitclass Object
(
    [publicProperty] => 1
)

Warning: runkit_default_property_remove(): Removing properties from internal classes is not allowed in /home/dzenovich/runkit-my/tests/runkit_default_property_remove4.php on line %d
