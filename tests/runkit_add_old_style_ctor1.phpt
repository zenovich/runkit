--TEST--
add old-style parent ctor (existing ctor)
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php

class Test {
}

class FOO_test extends test {
	function foo_test() {
		var_dump("foo_test ctor");
	}
}

class Foo_test_Child extends FOO_test{
}

class Test_Child extends Test{
}

class Test_GrandChild extends Test_Child {
	function test_grandchild() {
		var_dump("test_grandchild ctor");
	}
}

runkit_method_add("test", "test", "", "var_dump('new constructor');");
$a = new test;
$a = new foo_test;
$a = new FOO_test_Child;
$a = new Test_Child;
$a = new Test_GrandChild;

echo "after removing\n";
runkit_method_remove("test", "test");
$a = new test;
$a = new foo_test;
$a = new FOO_test_Child;
$a = new Test_Child;
$a = new Test_GrandChild;

echo "==DONE==\n";
?>
--EXPECT--
string(15) "new constructor"
string(13) "foo_test ctor"
string(13) "foo_test ctor"
string(15) "new constructor"
string(20) "test_grandchild ctor"
after removing
string(13) "foo_test ctor"
string(13) "foo_test ctor"
string(20) "test_grandchild ctor"
==DONE==
