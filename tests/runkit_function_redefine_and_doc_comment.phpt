--TEST--
runkit_function_redefine() function and doc_comment
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
display_errors=on
--FILE--
<?php
/**
 * old doc_comment
 */
function runkit_function($a) {
	echo "a is $a\n";
}
function runkitFunction($a) {
	echo "a is $a\n";
}
runkit_function_redefine('runkit_function','$b', 'echo "b is $b\n";', NULL, 'new doc_comment1');
runkit_function_redefine('runkitFunction','$b', 'echo "b is $b\n";', NULL, 'new doc_comment2');
$r1 = new ReflectionFunction('runkit_function');
echo $r1->getDocComment(), "\n";
$r2 = new ReflectionFunction('runkitFunction');
echo $r2->getDocComment(), "\n";
runkit_function_redefine('runkitFunction','$b', 'echo "b is $b\n";', NULL, NULL);
$r2 = new ReflectionFunction('runkitFunction');
echo $r2->getDocComment(), "\n";
runkit_function_redefine('runkitFunction','$b', 'echo "b is $b\n";');
$r2 = new ReflectionFunction('runkitFunction');
echo $r2->getDocComment(), "\n";
?>
--EXPECT--
new doc_comment1
new doc_comment2
new doc_comment2
new doc_comment2
