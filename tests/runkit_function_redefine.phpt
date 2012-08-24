--TEST--
runkit_function_redefine() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
$a = 'a';
function runkitSample($c) {
	global $a;
	echo "$a is $c\n";
}

$funcName = 'runkitSample';
runkitSample(1);
runkit_function_redefine($funcName,'$b','global $a; static $is="is"; for($i=0; $i<10; $i++) {} echo "$a $is $b\n";');
$a = 'b';
runkitSample(2);
echo $funcName
?>
--EXPECT--
a is 1
b is 2
runkitSample
