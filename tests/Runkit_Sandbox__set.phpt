--TEST--
Runkit_Sandbox::__set() method
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$test = 123;
$php->test = 321;
$php->eval('var_dump($test);');
var_dump($test);
--EXPECT--
int(321)
int(123)
