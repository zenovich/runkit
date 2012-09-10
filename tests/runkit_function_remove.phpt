--TEST--
runkit_function_remove() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function runkitSample() {
	echo "Function Exists\n";
}

$name = 'runkitSample';
runkitSample();
runkit_function_remove($name);
if (!function_exists('runkitSample')) {
	echo "Function Removed\n";
}
echo $name, "\n";
runkitSample();
?>
--EXPECTF--
Function Exists
Function Removed
runkitSample

Fatal error: %s runkit%sample() in %s on line %d
