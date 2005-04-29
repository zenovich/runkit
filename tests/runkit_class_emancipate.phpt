--TEST--
runkit_class_emancipate() function
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
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
class ChildClass extends ParentClass {
	function three() {
		echo "Three\n";
	}
}

var_dump(get_class_methods('ParentClass'));
var_dump(get_class_methods('ChildClass'));
runkit_class_emancipate('ParentClass');
var_dump(get_class_methods('ParentClass'));
var_dump(get_class_methods('ChildClass'));
?>
--EXPECT--
array(2) {
  [0]=>
  string(3) "two"
  [1]=>
  string(3) "one"
}
array(3) {
  [0]=>
  string(5) "three"
  [1]=>
  string(3) "two"
  [2]=>
  string(3) "one"
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
