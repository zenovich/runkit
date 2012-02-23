--TEST--
runkit_default_property_add() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=On
--FILE--
<?php
class RunkitClass {
    public $constArray = array();
    public $publicProperty = 1;
    public $publicproperty = 2;
    private $privateProperty = "a";
    protected $protectedProperty = "b";
}

ini_set('display_errors', 1);
ini_set('error_reporting', E_ALL);

$className = 'RunkitClass';
$obj = new $className();
runkit_default_property_add($className, 'dynamic', $obj);

runkit_default_property_remove($className, 'dynamic');
runkit_default_property_remove($className, 'publicproperty');
runkit_default_property_remove($className, 'publicproperty');
runkit_default_property_remove($className, 'privateProperty');
runkit_default_property_remove($className, 'protectedProperty');
runkit_default_property_remove($className, 'constArray');
print_r(new $className());

$obj = new stdClass();
runkit_default_property_remove('stdClass', 'str');
?>
--EXPECTF--
Warning: runkit_default_property_remove(): RunkitClass::publicproperty does not exist in /home/dzenovich/runkit-my/tests/runkit_default_property_remove.php on line %d
RunkitClass Object
(
    [publicProperty] => 1
)

Warning: runkit_default_property_remove(): Removing properties from internal classes is not allowed in /home/dzenovich/runkit-my/tests/runkit_default_property_remove.php on line %d
