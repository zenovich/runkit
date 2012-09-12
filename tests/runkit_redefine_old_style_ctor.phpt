--TEST--
redefine old-style parent ctor 
--FILE--
<?php

class Test {
	function test() {
		var_dump("original constructor");
	}
}

class FOO_test extends test {
}

runkit_method_redefine("test", "test", "", "var_dump('new constructor');");
$a = new foo_test;

echo "==DONE==\n";
?>
--EXPECT--
string(15) "new constructor"
==DONE==
