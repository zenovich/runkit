--TEST--
Runkit_Sandbox() - disable_classes test
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox(array('disable_classes'=>'stdClass'));
$php->ini_set('html_errors',false);
$php->eval('$a = new stdClass();');
--EXPECTF--
Fatal error: Class 'stdClass' not found in Unknown(0) : Runkit_Sandbox Eval Code on line 1

Warning: Runkit_Sandbox::eval(): Error executing sandbox code in %s on line %d
