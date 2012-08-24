--TEST--
Runkit_Sandbox::__call_user_func() method with closure as the first parameter
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip";
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip";
      if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$php->a = "OK\n";
$php->call_user_func(function() { global $a; print $a; }); /* As of PHP 5.3.0 */
--EXPECT--
OK
