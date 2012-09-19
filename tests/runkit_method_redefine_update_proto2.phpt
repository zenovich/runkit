--TEST--
runkit_method_redefine() must also update method's prototype
--SKIPIF--
<?php if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip"; ?>
--FILE--
<?php

class a {
	function foo() {
	}
}

runkit_method_redefine("a", "foo", "", "return false;");

eval("
class b extends a {
	function foo() {
	}
}");

echo "==DONE==\n";

?>
--EXPECT--
==DONE==
