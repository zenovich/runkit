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

class test {
	public function run() {
		$c = 'use';
		$d = 'ref_use';
		runkit_function_add('runkit_function',
			function($a, $b = "bar") use ($c, &$d) {
				static $is="is";
				global $g;
				echo "a $is $a\nb $is $b\n";
				echo "c $is $c\nd $is $d\n";
				echo "g $is $g\n";
				$d .= ' modified';
				echo '$this is';
				var_dump($this);
			}
		);
		runkit_function('foo', 'bar');
		echo "d after call is $d\n";
	}
}
$g = 'global';
$t = new test();
$t->run();
runkit_function('foo','bar');
?>
--EXPECTF--
a is foo
b is bar
c is use
d is ref_use
g is global
$this is
Notice: Undefined variable: this in %s on line %d
NULL
d after call is ref_use modified
a is foo
b is bar
c is use
d is ref_use modified
g is global
$this is
Notice: Undefined variable: this in %s on line %d
NULL
