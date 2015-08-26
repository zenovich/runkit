--TEST--
runkit_method_add() function with closure
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
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

class runkit_class {}

class test {
	public function run() {
		$c = 'use';
		$d = 'ref_use';
		runkit_method_add('runkit_class', 'runkit_method',
			function($a, $b = "bar") use ($c, &$d) {
			static $is="is";
			global $g;
			echo "a $is $a\nb $is $b\n";
			echo "c $is $c\nd $is $d\n";
			echo "g $is $g\n";
			$d .= ' modified';
			echo '$this is ';
			var_dump($this);
		});
		runkit_class::runkit_method('foo', 'bar');
		echo "d after call is $d\n";
	}
}
$g = 'global';
$t = new test();
$t->run();
$rc = new runkit_class();
$rc->runkit_method('foo','bar');
?>
--EXPECT--
a is foo
b is bar
c is use
d is ref_use
g is global
$this is object(test)#1 (0) {
}
d after call is ref_use modified
a is foo
b is bar
c is use
d is ref_use modified
g is global
$this is object(runkit_class)#2 (0) {
}
