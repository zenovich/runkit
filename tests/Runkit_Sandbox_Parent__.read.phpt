--TEST--
Runkit_Sandbox_Parent Class -- Read Access
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$one = 1;
$php = new Runkit_Sandbox();
$php['parent_access'] = true;
$php['parent_read'] = true;
$php->ini_set('html_errors',false);
$php->ini_set('display_errors',true);
$php->error_reporting(E_ALL);
$php->eval('$PARENT = new Runkit_Sandbox_Parent;
			var_dump($PARENT->one);');
--EXPECTF--
int(1)
