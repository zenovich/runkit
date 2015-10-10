--TEST--
runkit_function_add(), runkit_function_redefine(), runkit_function_rename() & runkit_function_copy() functions with namespaces
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php

namespace Test;
function bar() {
	echo "Called original bar()\n";
}

runkit_function_redefine('Test\bar', '', 'echo "Mocked\n";');
runkit_function_add('\Test\m', '', '');
runkit_function_redefine('\Test\m', '', 'echo "New mocked\n";');
runkit_function_add('Test\p', '', 'echo "New\n";');
runkit_function_rename('Test\m', '\Test\n');
runkit_function_rename('\Test\p', 'Test\s');
runkit_function_copy('\Test\n', 'Test\o');
runkit_function_copy('Test\s', '\Test\q');

bar();
o();
q();
\Test\bar();
\Test\o();
\Test\q();

runkit_function_remove('Test\n');
runkit_function_remove('\Test\s');
n();
?>
--EXPECTF--
Mocked
New mocked
New
Mocked
New mocked
New

Fatal error: Call to undefined function Test\n() in %s on line %d