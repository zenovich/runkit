--TEST--
runkit_default_property_add() function for static properties
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
    public static $oldProperty = 'old';
    public static function getProtected() {return self::$protectedProperty;}
    public static function getPrivate() {return self::$privateProperty;}
}

$className = 'RunkitClass';
$propName = 'publicProperty';
$value = 1;
$value2 = 'a';
runkit_default_property_add($className, 'constArray', array('a'=>1), RUNKIT_ACC_STATIC);
runkit_default_property_add($className, $propName, $value, RUNKIT_ACC_PUBLIC | RUNKIT_ACC_STATIC);
runkit_default_property_add($className, 'privateProperty', $value2, RUNKIT_ACC_PRIVATE | RUNKIT_ACC_STATIC);
runkit_default_property_add($className, 'protectedProperty', NULL, RUNKIT_ACC_PROTECTED | RUNKIT_ACC_STATIC);
$obj = new $className();
runkit_default_property_add($className, 'dynamic', $obj, RUNKIT_ACC_STATIC);
$value = 10;
$value2 = "b";
print_r(RunkitClass::$constArray);
var_dump(RunkitClass::$publicProperty);
var_dump(RunkitClass::getProtected());
var_dump(RunkitClass::getPrivate());
print_r(RunkitClass::$dynamic);
var_dump(RunkitClass::$oldProperty);
var_dump($className);
var_dump($propName);

runkit_default_property_add('stdClass', 'str', 'test');
var_dump(stdClass::$str);
?>
--EXPECTF--
Array
(
    [a] => 1
)
int(1)
NULL
string(1) "a"
RunkitClass Object
(
)
string(3) "old"
string(11) "RunkitClass"
string(14) "publicProperty"

Warning: runkit_default_property_add(): Adding properties to internal classes is not allowed in %s on line %d

Fatal error: Access to undeclared static property: stdClass::$str in %s on line %d
