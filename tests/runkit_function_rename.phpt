--TEST--
runkit_function_rename() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function runkitSample($n) {
	echo "Runkit Sample: $n\n";
}

$oldName = 'runkitSample';
$newName = 'runkitNewName';
runkitSample(1);
runkit_function_rename($oldName, $newName);
if (function_exists('runkitSample')) {
	echo "Old function name still exists!\n";
}
runkitNewName(2);
echo $oldName, "\n";
echo $newName, "\n";

runkitSample(2);
?>
--EXPECTF--
Runkit Sample: 1
Runkit Sample: 2
runkitSample
runkitNewName

Fatal error: %s runkit%sample() in %s on line %d
