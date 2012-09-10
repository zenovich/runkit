--TEST--
Runkit_Sandbox['active'] status
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip";
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
var_dump($php['active']);
echo $php->die("What a world...\n");
var_dump($php['active']);
--EXPECT--
bool(true)
What a world...
bool(false)
