--TEST--
runkit_sandbox_output_handler() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_sandbox_output_handler")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
runkit_sandbox_output_handler($php, 'test_handler');
$php->echo("foo\n");
$php->echo("Barish\n");
$php->echo("BAZimbly\n");

function test_handler($str) {
  if (strlen($str) == 0) return NULL; /* flush() */
  /* Echoing and returning have the same effect here, both go to parent's output chain */
  echo 'Received string from sandbox: ' . strlen($str) . " bytes long.\n";

  return strtoupper($str);
}
--EXPECTF--
Notice: runkit_sandbox_output_handler(): Use of runkit_sandbox_output_handler() is deprecated.  Use $sandbox['output_handler'] instead. in %s on line %d
Received string from sandbox: 4 bytes long.
FOO
Received string from sandbox: 7 bytes long.
BARISH
Received string from sandbox: 9 bytes long.
BAZIMBLY
