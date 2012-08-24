--TEST--
runkit.superglobal setting
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
display_errors=on
runkit.superglobal=foo,bar
--FILE--
<?php
function testme() {
	echo "Foo is $foo\n";
	echo "Bar is $bar\n";
	echo "Baz is $baz\n";
}

$v = explode(".", PHP_VERSION);
if (array_shift($v) >= 5) {
	if (!defined('E_STRICT')) {
		define('E_STRICT', 0);
	}
	if (!defined('E_DEPRECATED')) {
		define('E_DEPRECATED', 0);
	}
	ini_set('error_reporting', E_ALL & (~E_DEPRECATED) & (~E_STRICT));
}

$foo = 1;
$bar = 2;
$baz = 3;

testme();
--EXPECTF--
Foo is 1
Bar is 2

Notice: Undefined variable: %wbaz in %s on line %d
Baz is
