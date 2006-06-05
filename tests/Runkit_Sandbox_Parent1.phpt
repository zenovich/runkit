--TEST--
Runkit_Sandbox_Parent Class -- Instantiation from outter scope
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
var_dump(new Runkit_Sandbox_Parent());
--EXPECTF--
object(stdClass)#%d (0) {
}

