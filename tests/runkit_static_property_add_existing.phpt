--TEST--
runkit_default_property_add() function for existing static properties
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class RunkitParent {
}

class RunkitClass extends RunkitParent {
    public static $oldProperty = 'old';
}

class RunkitSubClass extends RunkitClass {
}

runkit_default_property_add('RunkitClass', 'oldProperty', array('a'=>1), RUNKIT_ACC_STATIC);
var_dump(RunkitClass::$oldProperty);
runkit_default_property_add('RunkitSubClass', 'oldProperty', array('a'=>1), RUNKIT_ACC_STATIC);
var_dump(RunkitSubClass::$oldProperty);
runkit_default_property_add('RunkitParent', 'oldProperty', array('a'=>1), RUNKIT_ACC_STATIC);
var_dump(RunkitParent::$oldProperty);
var_dump(RunkitClass::$oldProperty);
var_dump(RunkitSubClass::$oldProperty);
?>
--EXPECTF--

Warning: runkit_default_property_add(): RunkitClass::$oldProperty already exists in %s on line %d
string(3) "old"

Warning: runkit_default_property_add(): RunkitSubClass::$oldProperty already exists in %s on line %d
string(3) "old"

Notice: runkit_default_property_add(): RunkitClass::$oldProperty already exists, not adding in %s on line %d
array(1) {
  ["a"]=>
  int(1)
}
string(3) "old"
string(3) "old"
