--TEST--
runkit_method_redefine() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class runkit_class {
	function runkit_method($a) {
		echo "a is $a\n";
	}
}
runkit_class::runkit_method('foo');
runkit_method_redefine('runkit_class','runkit_method','$b', 'echo "b is $b\n";');
runkit_class::runkit_method('bar');
?>
--EXPECT--
a is foo
b is bar
