--TEST--
runkit_import() Importing and overriding non-static and static properties with static properties
--SKIPIF--
<?php
	if (!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) {
		echo "skip";
	}
	if (array_shift(explode(".", PHP_VERSION)) < 5) {
		echo "skip";
	}
?>
--INI--
display_errors=on
--FILE--
<?php
class Test {
    public $n = 1;
    public static $s = 2;
}

if (!defined('E_STRICT')) {
	define('E_STRICT', 0);
}
if (!defined('E_DEPRECATED')) {
	define('E_DEPRECATED', 0);
}
ini_set('error_reporting', E_ALL & ~E_DEPRECATED & ~E_STRICT);

$t = new Test;
var_dump($t->n);
var_dump(Test::$s);
runkit_import(dirname(__FILE__) . '/runkit_import_static_properties_override.inc', RUNKIT_IMPORT_CLASS_STATIC_PROPS | RUNKIT_IMPORT_OVERRIDE);
$t = new Test;
var_dump($t->n);
var_dump(Test::$n);
var_dump(Test::$s);
?>
--EXPECTF--
int(1)
int(2)

Notice: Undefined property: Test::$n in %s on line %d
NULL
int(3)
int(4)
