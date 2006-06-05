--TEST--
Runkit_Sandbox::eval() method
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$php->eval('var_dump(isset($php)); $test = 123; echo "$test\n";');
var_dump(isset($test));
--EXPECT--
bool(false)
123
bool(false)
