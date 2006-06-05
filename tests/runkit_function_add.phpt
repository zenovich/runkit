--TEST--
runkit_function_add() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
runkit_function_add('runkit_sample', '$a, $b, $c = "baz"', 'echo "a is $a\nb is $b\nc is $c\n";');
runkit_sample('foo','bar');
?>
--EXPECT--
a is foo
b is bar
c is baz
