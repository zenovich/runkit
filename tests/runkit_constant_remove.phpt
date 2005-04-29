--TEST--
runkit_constant_remove() function
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
define('FOO', "BAR\n");
echo FOO;
runkit_constant_remove('FOO');
if (!defined('FOO')) {
	echo "BAZ\n";
}
?>
--EXPECT--
BAR
BAZ
