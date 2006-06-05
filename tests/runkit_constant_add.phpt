--TEST--
runkit_constant_add() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
runkit_constant_add('FOO', "BAR\n");
echo FOO;
?>
--EXPECT--
BAR
