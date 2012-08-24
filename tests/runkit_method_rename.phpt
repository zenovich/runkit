--TEST--
runkit_method_rename() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
display_errors=on
--FILE--
<?php
if (!defined('E_STRICT')) {
	define('E_STRICT', 0);
}
if (!defined('E_DEPRECATED')) {
	define('E_DEPRECATED', 0);
}
ini_set('error_reporting', E_ALL & (~E_DEPRECATED) & (~E_STRICT));

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
runkit_method_rename('runkit_class','runkit_duplicate', 'runkitDuplicate');
if (method_exists('runkit_class','runkit_duplicate')) {
	echo "Runkit Duplicate still exists!\n";
}
runkit_class::runkitDuplicate(3);
runkit_method_rename('runkit_class','runkitDuplicate', 'runkit_original');
if (method_exists('runkit_class','runkitDuplicate')) {
	echo "RunkitDuplicate still exists!\n";
}
runkit_class::runkit_original(4);
runkit_class::runkitDuplicate(4);
?>
--EXPECTF--
Runkit Original: a is 1
Runkit Original: a is 2
Runkit Original: a is 3
Runkit Original: a is 4

Fatal error: Call to undefined %s runkit_class::runkit%suplicate() in %s on line %d
