--TEST--
add old-style parent ctor (existing ctor)
--FILE--
<?php

class Test {
}

class FOO_test extends test {
	function foo_test() {
		var_dump("foo_test ctor");
	}
}

runkit_method_add("test", "test", "", "var_dump('new constructor');");
$a = new foo_test;

echo "==DONE==\n";
?>
--EXPECT--
string(13) "foo_test ctor"
==DONE==
