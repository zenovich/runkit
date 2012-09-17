--TEST--
runkit_function_redefine() & runkit_function_add() for functions returning a value by reference
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
if (!defined('E_STRICT')) {
	define('E_STRICT', 0);
}
if (!defined('E_DEPRECATED')) {
	define('E_DEPRECATED', 0);
}
ini_set('error_reporting', E_ALL & (~E_DEPRECATED) & (~E_STRICT) & (~E_NOTICE));

$a = 0;

function runkitSample() {
	global $a;
	return $a;
}
$code = 'global $a; return $a;';

$b = &runkitSample();
$b = 1;
var_dump($a);

runkit_function_redefine('runkitSample', '', $code);
$c = &runkitSample();
$c = 2;
var_dump($a);

runkit_function_redefine('runkitSample', '', $code, TRUE);
$r = &runkitSample();
$r = 3;
var_dump($a);

runkit_function_redefine('runkitSample', '', $code, FALSE);
$d = &runkitSample();
$d = 4;
var_dump($a);

runkit_function_remove('runkitSample');
runkit_function_add('runkitSample', '', $code);
$d = &runkitSample();
$d = 5;
var_dump($a);

runkit_function_remove('runkitSample');
runkit_function_add('runkitSample', '', $code, TRUE);
$d = &runkitSample();
$d = 6;
var_dump($a);
?>
--EXPECT--
int(0)
int(0)
int(3)
int(3)
int(3)
int(6)
