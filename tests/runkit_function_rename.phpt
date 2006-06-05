--TEST--
runkit_function_rename() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function runkit_sample($n) {
	echo "Runkit Sample: $n\n";
}

runkit_sample(1);
runkit_function_rename('runkit_sample','runkit_newname');
if (function_exists('runkit_sample')) {
	echo "Old function name still exists!\n";
}
runkit_newname(2);
?>
--EXPECT--
Runkit Sample: 1
Runkit Sample: 2
