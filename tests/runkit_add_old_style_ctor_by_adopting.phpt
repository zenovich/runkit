--TEST--
add old-style parent ctor by adoptting
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php

class Father {
	function teSt() {
		var_dump('new constructor');
	}
}

class Test {
}

class FOO_test extends test {
}

class FOO_test_Child extends FOO_test {
}

runkit_class_adopt("Test", "Father");
$a = new test;
$a = new foo_test;
$a = new FOO_test_Child;

runkit_class_emancipate("Test");
echo "after emancipation\n";
$a = new test;
$a = new foo_test;
$a = new FOO_test_Child;

?>
--EXPECT--
string(15) "new constructor"
string(15) "new constructor"
string(15) "new constructor"
after emancipation
