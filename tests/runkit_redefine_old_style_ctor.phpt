--TEST--
redefine old-style parent ctor
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php

class Test {
	function test() {
		var_dump("original constructor");
	}
}

class FOO_test extends test {
}

class FOO_test_child extends FOO_test {
}

class FOO_test_child_changed extends Foo_test_child {
	function Foo_test_child_changed() {
		var_dump("FOO_test_child_changed constructor");
	}
}

class FOO_test_child_changed_child extends FOO_test_child_changed {
}

runkit_method_redefine("test", "test", "", "var_dump('new constructor');");
$a = new test;
$a = new foo_test;
$a = new foo_test_child;
$a = new foo_test_child_changed;
$a = new foo_test_child_changed_child;

echo "after renaming\n";
runkit_method_rename("test", "test", "test1");
$a = new test;
$a = new foo_test;
$a = new foo_test_child;
$a = new foo_test_child_changed;
$a = new foo_test_child_changed_child;

echo "\n";
$a = new foo_test;
$a->test1();
echo "==DONE==\n";
?>
--EXPECT--
string(15) "new constructor"
string(15) "new constructor"
string(15) "new constructor"
string(34) "FOO_test_child_changed constructor"
string(34) "FOO_test_child_changed constructor"
after renaming
string(34) "FOO_test_child_changed constructor"
string(34) "FOO_test_child_changed constructor"

string(15) "new constructor"
==DONE==
