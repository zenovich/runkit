--TEST--
runkit_method_redefine() function and doc_comment
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
	/**
	 * old doc_comment
	 */
	function runkit_method($a) {
		echo "a is $a\n";
	}
	function runkitMethod($a) {
		echo "a is $a\n";
	}
}
runkit_method_redefine('runkit_class','runkit_method','$b', 'echo "b is $b\n";', NULL, 'new doc_comment1');
runkit_method_redefine('runkit_class','runkitMethod','$b', 'echo "b is $b\n";', NULL, 'new doc_comment2');
$r1 = new ReflectionMethod('runkit_class', 'runkit_method');
echo $r1->getDocComment(), "\n";
$r2 = new ReflectionMethod('runkit_class', 'runkitMethod');
echo $r2->getDocComment(), "\n";
?>
--EXPECT--
new doc_comment1
new doc_comment2
