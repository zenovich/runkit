--TEST--
Runkit_Sandbox['output_handler'] setting
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_sandbox_output_handler")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$php['output_handler'] = 'test_handler';
$php->echo("foo\n");
$php->echo("Barish\n");
$php->echo("BAZimbly\n");
var_dump($php['output_handler']);

function test_handler($str) {
  if (strlen($str) == 0) return NULL; /* Do nothing with flush events */
  /* Echoing and returning have the same effect here, both go to parent's output chain */
  echo 'Received string from sandbox: ' . strlen($str) . " bytes long.\n";

  return strtoupper($str);
}
--EXPECT--
Received string from sandbox: 4 bytes long.
FOO
Received string from sandbox: 7 bytes long.
BARISH
Received string from sandbox: 9 bytes long.
BAZIMBLY
string(12) "test_handler"
