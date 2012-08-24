--TEST--
runkit_method_add(), runkit_method_redefine(), runkit_method_rename() & runkit_method_copy() functions with namespaces
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
class Foo {
	public function bar() {
		echo "Called original bar()\n";
	}
}

runkit_method_redefine('Test\Foo', 'bar', '', 'echo "Mocked\n";');
runkit_method_add('Test\Foo', 'm', '', '');
runkit_method_redefine('\Test\Foo', 'm', '', 'echo "New mocked\n";');
runkit_method_add('\Test\Foo', 'p', '', 'echo "New\n";');
runkit_method_rename('Test\Foo', 'm', 'n');
runkit_method_rename('\Test\Foo', 'p', 's');
runkit_method_copy('Test\Foo', 'o', 'Test\Foo', 'n');
runkit_method_copy('\Test\Foo', 'q', '\Test\Foo', 's');

$foo = new \Test\Foo();
$foo->bar();
$foo->o();
$foo->q();

runkit_method_remove('Test\Foo', 'n');
runkit_method_remove('\Test\Foo', 's');
$foo->n();
?>
--EXPECTF--
Mocked
New mocked
New

Fatal error: Call to undefined method Test\Foo::n() in %s on line %d
