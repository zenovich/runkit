--TEST--
runkit_class_emancipate() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
class Grandparent {
	function one() {
		echo "One\n";
	}
}
class ParentClass extends Grandparent {
	function two() {
		echo "Two\n";
	}
}
class ParentClass1 extends Grandparent {
}
class ChildClass extends ParentClass {
	function three() {
		echo "Three\n";
	}
}

$t = get_class_methods('ParentClass');
sort($t);
var_dump($t);
$t = get_class_methods('ChildClass');
sort($t);
var_dump($t);
runkit_class_emancipate('ParentClass');
$t = get_class_methods('ParentClass');
sort($t);
var_dump($t);
$t = get_class_methods('ChildClass');
sort($t);
var_dump($t);
$t = get_class_methods('ParentClass1');
sort($t);
var_dump($t);
?>
--EXPECT--
array(2) {
  [0]=>
  string(3) "one"
  [1]=>
  string(3) "two"
}
array(3) {
  [0]=>
  string(3) "one"
  [1]=>
  string(5) "three"
  [2]=>
  string(3) "two"
}
array(1) {
  [0]=>
  string(3) "two"
}
array(2) {
  [0]=>
  string(5) "three"
  [1]=>
  string(3) "two"
}
array(1) {
  [0]=>
  string(3) "one"
}
