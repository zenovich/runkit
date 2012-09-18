--TEST--
add old-style parent ctor by copying
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php

class Test {
	function test1() {
		var_dump('new constructor');
	}
}

class FOO_test extends test {
}

class FOO_test_Child extends FOO_test {
}

runkit_method_copy("test", "test", "test", "test1");
$a = new test;
$a = new foo_test;
$a = new FOO_test_Child;

echo "after removing\n";

runkit_method_remove("test", "test");
$a = new test;
$a = new foo_test;
$a = new FOO_test_Child;

echo "==DONE==\n";
?>
--EXPECT--
string(15) "new constructor"
string(15) "new constructor"
string(15) "new constructor"
after removing
==DONE==
