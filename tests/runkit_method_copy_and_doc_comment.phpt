--TEST--
runkit_method_copy() function and doc_comment
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
}
runkit_method_copy('runkit_class','runkit_method1', 'runkit_class', 'runkit_method');
$r = new ReflectionMethod('runkit_class', 'runkit_method1');
echo $r->getDocComment(), "\n";
runkit_method_redefine('runkit_class','runkit_method','', '', NULL, 'new doc_comment');
$r = new ReflectionMethod('runkit_class', 'runkit_method1');
echo $r->getDocComment(), "\n";
$r = new ReflectionMethod('runkit_class', 'runkit_method');
echo $r->getDocComment(), "\n";
echo "After redefine\n";
runkit_method_redefine('runkit_class','runkit_method','', '', NULL, NULL);
$r = new ReflectionMethod('runkit_class', 'runkit_method');
echo $r->getDocComment(), "\n";
echo "After redefine 2\n";
runkit_method_redefine('runkit_class','runkit_method','', '');
$r = new ReflectionMethod('runkit_class', 'runkit_method');
echo $r->getDocComment(), "\n";
echo "After redefine 3\n";
runkit_method_redefine('runkit_class','runkit_method','', '', NULL, '');
$r = new ReflectionMethod('runkit_class', 'runkit_method');
echo $r->getDocComment(), "\n";
?>
--EXPECT--
/**
	 * old doc_comment
	 */
/**
	 * old doc_comment
	 */
new doc_comment
After redefine
new doc_comment
After redefine 2
new doc_comment
After redefine 3

