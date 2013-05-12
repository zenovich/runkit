--TEST--
runkit_default_property_add() add properties to subclasses overriding objects
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
	public function setPrivate() {$this->privateProperty = "b";}
	public function setProtected() {$this->protectedProperty = 1;}
}

class RunkitSubClass extends RunkitClass {}
class StdSubClass extends stdClass {}

$className = 'RunkitClass';
$propName = 'publicProperty';
$parentObj = new $className;
runkit_default_property_add($className, 'constArray', array('a'=>1), RUNKIT_OVERRIDE_OBJECTS);
runkit_default_property_add($className, $propName, 1, RUNKIT_ACC_PUBLIC | RUNKIT_OVERRIDE_OBJECTS);
runkit_default_property_add($className, 'privateProperty', "a", RUNKIT_ACC_PRIVATE | RUNKIT_OVERRIDE_OBJECTS);
runkit_default_property_add($className, 'protectedProperty', NULL, RUNKIT_ACC_PROTECTED | RUNKIT_OVERRIDE_OBJECTS);
$obj = new RunkitSubClass;
runkit_default_property_add($className, 'dynamic', $obj, RUNKIT_OVERRIDE_OBJECTS);

$parentObj->constArray = array('b'=>2);
$parentObj->publicProperty = 2;
$parentObj->setPrivate();
$parentObj->setProtected();
$parentObj->dynamic = $parentObj;

print_r($obj);
print_r(new RunkitSubClass);

$obj = new StdSubClass();
runkit_default_property_add('StdSubClass', 'str', 'test', RUNKIT_OVERRIDE_OBJECTS);
print_r($obj);
?>
--EXPECTF--
RunkitSubClass Object
(
    [constArray] => Array
        (
            [a] => 1
        )

    [publicProperty] => 1
    [privateProperty%sprivate] => a
    [protectedProperty:protected] =>%w
%w[dynamic] => RunkitSubClass Object
%w*RECURSION*%w
)
RunkitSubClass Object
(
    [constArray] => Array
        (
            [a] => 1
        )

    [publicProperty] => 1
    [privateProperty%sprivate] => a
    [protectedProperty:protected] =>%w
    [dynamic] => RunkitSubClass Object
        (
            [constArray] => Array
                (
                    [a] => 1
                )

            [publicProperty] => 1
            [privateProperty%sprivate] => a
            [protectedProperty:protected] =>%w
%w[dynamic] => RunkitSubClass Object
%w*RECURSION*%w
        )

)
StdSubClass Object
(
    [str] => test
)
