--TEST--
Runkit_Sandbox::__call() method
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
echo $php->str_replace('foo','bar',"The word of the day is foo\n");
--EXPECT--
The word of the day is bar
