--TEST--
runkit_method_remove() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class runkit_class {
	function runkit_method() {
		echo "Runkit Method\n";
	}
	function runkitMethod() {
		echo "Runkit Method\n";
	}
}

runkit_class::runkit_method();
runkit_method_remove('runkit_class','runkit_method');
if (!method_exists('runkit_class','runkit_method')) {
	echo "Runkit Method Removed\n";
}
runkit_class::runkitMethod();
runkit_method_remove('runkit_class','runkitMethod');
if (!method_exists('runkit_class','runkitMethod')) {
	echo "Runkit Method Removed\n";
}
?>
--EXPECT--
Runkit Method
Runkit Method Removed
Runkit Method
Runkit Method Removed
