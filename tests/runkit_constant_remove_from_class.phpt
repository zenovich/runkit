--TEST--
runkit_constant_remove() function removes constant from class
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class TestClass {
	const FOO = 'foo';
}

$const = 'TestClass::FOO';
var_dump($const);
runkit_constant_remove($const);
var_dump($const);
var_dump(TestClass::FOO);
?>
--EXPECTF--
string(14) "TestClass::FOO"
string(14) "TestClass::FOO"

Fatal error: Undefined class constant 'FOO' in %s on line %d
