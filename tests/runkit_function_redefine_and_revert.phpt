--TEST--
runkit_function_redefine() and revert
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
runkit.internal_override=On
--FILE--
<?php
//xdebug_start_code_coverage();
echo sprintf("%s\n",'bar');
runkit_function_copy('sprintf','sprintf_old');
runkit_function_redefine('sprintf','$a,$b', 'return "new function\n" . sprintf_old($a,$b);');
echo sprintf("%s\n",'bar');
runkit_function_remove('sprintf');
runkit_function_copy('sprintf_old','sprintf');
runkit_function_remove('sprintf_old');
echo sprintf("%s\n",'foo');
?>
--EXPECT--
bar
new function
bar
foo
