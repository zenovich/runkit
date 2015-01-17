--TEST--
Bug#57249 - Sutdown bug with runkit_import on a function-static variable
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
runkit_import('bug57249.inc', RUNKIT_IMPORT_CLASS_METHODS);
$g_oBuggyObject = new cBuggyClass();
$g_oBuggyObject->mBuggyMethod();

class cBuggyClass { }

--EXPECT--
mBuggyMethod();
