--TEST--
runkit_function_rename() function on internal functions
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
runkit.internal_override="1"
--FILE--
<?php
$a = 1;
var_dump($a);
runkit_function_rename('var_dump','qwerty');
if (function_exists('var_dump')) {
	echo "Old function name still exists!\n";
}
qwerty($a);
?>
--EXPECT--
int(1)
int(1)
