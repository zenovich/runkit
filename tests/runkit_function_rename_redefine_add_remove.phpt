--TEST--
complex test for renaming, adding and removing with internal functions
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
runkit.internal_override=On
--FILE--
<?php
runkit_function_rename('rand', 'oldRand');
echo oldRand(), "\n";
runkit_function_add('rand', '', 'return "a" . oldRand();');
echo rand(), "\n";
runkit_function_remove('rand');
runkit_function_rename('oldRand', 'rand');
echo rand(), "\n";
echo "\n";
// once again
runkit_function_rename('rand', 'oldRand');
echo oldRand(), "\n";
runkit_function_add('rand', '', 'return "a" . oldRand();');
echo rand(), "\n";
runkit_function_remove('rand');
runkit_function_rename('oldRand', 'rand');
echo rand(), "\n";
?>
--EXPECTF--
%d
a%d
%d

%d
a%d
%d
