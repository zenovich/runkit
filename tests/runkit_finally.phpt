--TEST--
copy method with finally
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(version_compare(PHP_VERSION, '5.5.0', '<')) print "skip";
?>
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
