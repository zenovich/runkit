--TEST--
runkit_method_rename() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class runkit_class {
	function runkit_original($a) {
		echo "Runkit Original: a is $a\n";
	}
}

runkit_class::runkit_original(1);
runkit_method_rename('runkit_class','runkit_original','runkit_duplicate');
if (method_exists('runkit_class','runkit_original')) {
	echo "Runkit Original still exists!\n";
}
runkit_class::runkit_duplicate(2);
?>
--EXPECT--
Runkit Original: a is 1
Runkit Original: a is 2
