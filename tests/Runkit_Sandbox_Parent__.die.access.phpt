--TEST--
Runkit_Sandbox_Parent Class -- Locked Patricide
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
echo "Before die()\n";
$php->eval('$PARENT = new Runkit_Sandbox_Parent;
			$PARENT->die();');
echo "After die()\n";
--EXPECT--
Before die()

Warning: Runkit_Sandbox_Parent::die(): Patricide is disabled.  Shame on you Oedipus. in Unknown(0) : Runkit_Sandbox Eval Code on line 2
After die()
