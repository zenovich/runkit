--TEST--
Runkit_Sandbox() - disable_functions test
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox(array('disable_functions'=>'php_sapi_name'));
$php->ini_set('html_errors',false);
$php->eval('php_sapi_name();');
--EXPECT--
Warning: php_sapi_name() has been disabled for security reasons in Unknown(0) : Runkit_Sandbox Eval Code on line 1

