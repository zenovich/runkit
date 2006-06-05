--TEST--
runkit_lint() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip";
      /* Note: It's not currently possible to change the value of html_errors in a subinterpreter from SYSTEM settings.... */
?>
--FILE--
<?php
var_dump(runkit_lint('echo "Foo";'));
var_dump(runkit_lint('echo "Bar;'));
--EXPECTF--
bool(true)
%s
%s error%s:%s error, unexpected $end in %sUnknown(0) : runkit_lint test compile%s on line %s1%s
bool(false)
