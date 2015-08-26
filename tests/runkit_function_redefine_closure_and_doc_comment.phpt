--TEST--
runkit_function_redefine() closure and doc_comment
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--INI--
display_errors=on
--FILE--
<?php
function runkit_function() {}

runkit_function_redefine('runkit_function', function () {}, 'new doc_comment');
$r1 = new ReflectionFunction('runkit_function');
echo $r1->getDocComment(), "\n";
?>
--EXPECT--
new doc_comment
