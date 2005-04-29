--TEST--
runkit_constant_add() function
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
runkit_constant_add('FOO', "BAR\n");
echo FOO;
?>
--EXPECT--
BAR
