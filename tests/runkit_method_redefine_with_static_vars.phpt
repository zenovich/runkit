--TEST--
redefining methods with static variables
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
display_errors=on
--FILE--
<?php
if (!defined('E_STRICT')) {
	define('E_STRICT', 0);
}
if (!defined('E_DEPRECATED')) {
	define('E_DEPRECATED', 0);
}
ini_set('error_reporting', E_ALL & (~E_DEPRECATED) & (~E_STRICT));


class A {
    function m() {
        static $a = 0;
        $a++;
        return $a;
    }
}

echo A::m(), "\n";

runkit_method_copy('A', 'm1', 'A', 'm');
runkit_method_remove('A', 'm');

echo A::m1();
?>
--EXPECT--
1
2
