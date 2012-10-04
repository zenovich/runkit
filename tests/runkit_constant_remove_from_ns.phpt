--TEST--
runkit_constant_remove() and namespaces 
--FILE--
<?php

namespace Mock\Constant\Command;

define('Mock\Constant\Command\Test', 'test1');
var_dump(runkit_constant_remove('Mock\Constant\Command\Test'));
var_dump(runkit_constant_add('Mock\Constant\Command\Test', 'test1'));
var_dump(runkit_constant_remove('Mock\Constant\Command\Test'));
var_dump(runkit_constant_add('Mock\Constant\Command\Test', 'test1'));

echo "==DONE==\n";

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
==DONE==
