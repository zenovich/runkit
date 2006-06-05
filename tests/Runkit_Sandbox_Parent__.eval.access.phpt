--TEST--
Runkit_Sandbox_Parent Class -- Locked Eval
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$php['parent_access'] = true;
$php->ini_set('html_errors',false);
$php->ini_set('display_errors',true);
$php->error_reporting(E_ALL);
$php->eval('$PARENT = new Runkit_Sandbox_Parent;
			$PARENT->eval(\'var_dump($php);\');');
--EXPECT--
Warning: Runkit_Sandbox_Parent::eval(): Access to eval() code in the parent context is not enabled in Unknown(0) : Runkit_Sandbox Eval Code on line 2
