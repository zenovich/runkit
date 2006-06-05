--TEST--
runkit_function_redefine() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function runkit_sample($a) {
	echo "a is $a\n";
}

runkit_sample(1);
runkit_function_redefine('runkit_sample','$b','echo "b is $b\n";');
runkit_sample(2);
?>
--EXPECT--
a is 1
b is 2
