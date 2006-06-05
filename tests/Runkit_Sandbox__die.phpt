--TEST--
Runkit_Sandbox::die() method
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$php->ini_set('html_errors',false);
echo $php->echo('foo');
echo $php->die();
echo $php->echo('bar');
--EXPECTF--
foo
Warning: Runkit_Sandbox::echo(): Current sandbox is no longer active in %s on line %d
