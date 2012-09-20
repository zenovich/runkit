--TEST--
Superglobals should not be wiped by runkit
--SKIPIF--
<?php
if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
runkit.superglobal=_POST,_REQUEST,my
--POST--
TEST=test
--FILE--
<?php
echo $_POST['TEST'], "\n";
echo $_REQUEST['TEST'], "\n";
--EXPECT--
test
test
