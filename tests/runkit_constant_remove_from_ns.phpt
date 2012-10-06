--TEST--
runkit_constant_remove(), runkit_constant_add(), and namespaces
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--FILE--
<?php

namespace Mock\Constant\Command;
class RunkitClass {const Test='test';}

define('Mock\Constant\Command\Test', 'test');
var_dump(\Mock\Constant\Command\Test);
var_dump(runkit_constant_remove('Mock\Constant\Command\Test'));
var_dump(defined('\Mock\Constant\Command\Test'));
var_dump(runkit_constant_add('Mock\Constant\Command\Test', 'test1'));
var_dump(\Mock\Constant\Command\Test);
var_dump(runkit_constant_remove('Mock\Constant\Command\Test'));
var_dump(defined('\Mock\Constant\Command\Test'));
var_dump(runkit_constant_add('Mock\Constant\Command\Test', 'test2'));
var_dump(\Mock\Constant\Command\Test);

echo "\n";

var_dump(\Mock\Constant\Command\RunkitClass::Test);
var_dump(runkit_constant_remove('Mock\Constant\Command\RunkitClass::Test'));
var_dump(defined('\Mock\Constant\Command\RunkitClass::Test'));
var_dump(runkit_constant_add('Mock\Constant\Command\RunkitClass::Test', 'test1'));
var_dump(\Mock\Constant\Command\RunkitClass::Test);
var_dump(runkit_constant_remove('Mock\Constant\Command\RunkitClass::Test'));
var_dump(defined('\Mock\Constant\Command\RunkitClass::Test'));
var_dump(runkit_constant_add('Mock\Constant\Command\RunkitClass::Test', 'test2'));
var_dump(\Mock\Constant\Command\RunkitClass::Test);

echo "\n";

define('Test', 'test');
var_dump(\Test);
var_dump(runkit_constant_remove('\Test'));
var_dump(defined('\Test'));
var_dump(runkit_constant_add('\Test', 'test1'));
var_dump(\Test);
var_dump(runkit_constant_remove('\Test'));
var_dump(defined('\Test'));
var_dump(runkit_constant_add('\Test', 'test2'));
var_dump(\Test);

echo "==DONE==\n";
?>
--EXPECT--
string(4) "test"
bool(true)
bool(false)
bool(true)
string(5) "test1"
bool(true)
bool(false)
bool(true)
string(5) "test2"

string(4) "test"
bool(true)
bool(false)
bool(true)
string(5) "test1"
bool(true)
bool(false)
bool(true)
string(5) "test2"

string(4) "test"
bool(true)
bool(false)
bool(true)
string(5) "test1"
bool(true)
bool(false)
bool(true)
string(5) "test2"
==DONE==
