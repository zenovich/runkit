--TEST--
runkit_function_remove() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function runkit_sample() {
	echo "Function Exists\n";
}

runkit_sample();
runkit_function_remove('runkit_sample');
if (!function_exists('runkit_sample')) {
	echo "Function Removed\n";
}
?>
--EXPECT--
Function Exists
Function Removed
