--TEST--
runkit_import() Importing and overriding property with constant array as the value
--SKIPIF--
<?php
    if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
    if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip"
?>
--FILE--
<?php
class Test {
    const C = 1;
    public $v = array(Test::C,2,3);
}
class Test1 extends Test {
}
$t = new Test;
var_dump($t->v);
runkit_import(dirname(__FILE__) . '/runkit_import_constant_properties.inc', RUNKIT_IMPORT_CLASS_PROPS | RUNKIT_IMPORT_OVERRIDE);
$t = new Test;
var_dump($t->v);
$t = new Test1;
var_dump($t->v);
runkit_import(dirname(__FILE__) . '/runkit_import_constant_properties.inc', RUNKIT_IMPORT_CLASSES | RUNKIT_IMPORT_OVERRIDE);
$t = new Test;
var_dump($t->v);
$t = new Test1;
var_dump($t->v);
?>
--EXPECT--
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(5)
  [2]=>
  int(6)
}
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(5)
  [2]=>
  int(6)
}
array(3) {
  [0]=>
  int(4)
  [1]=>
  int(5)
  [2]=>
  int(6)
}
array(3) {
  [0]=>
  int(4)
  [1]=>
  int(5)
  [2]=>
  int(6)
}
