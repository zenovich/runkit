--TEST--
copy method with finally 
--FILE--
<?php

class Test {

	function foo() {
		try {

		} finally {
		}
		echo "test\n";
	}
}

runkit_method_copy("Test", "bar", "Test", "foo");
runkit_method_remove("Test", "foo");

$o = new Test;
$o->bar();

?>
--EXPECTF--
test
