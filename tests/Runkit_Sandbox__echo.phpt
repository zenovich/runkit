--TEST--
Runkit_Sandbox::echo() method
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
echo $php->echo("foo\n","bar\n","baz\n");
--EXPECT--
foo
bar
baz
