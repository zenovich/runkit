--TEST--
runkit_constant_redefine() function redefines class constants
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
runkit_constant_redefine($const, 'bar');
var_dump($const, TestClass::FOO);
?>
--EXPECT--
string(14) "TestClass::FOO"
string(14) "TestClass::FOO"
string(3) "bar"
