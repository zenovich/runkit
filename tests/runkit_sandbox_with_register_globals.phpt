--TEST--
Runkit_Sandbox with register_globals
--SKIPIF--
<?php
  if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) echo "skip";
  if(version_compare(PHP_VERSION, '5.3.999', '>')) echo "skip";
?>
--INI--
register_globals=On
--FILE--
<?php
$php = new Runkit_Sandbox();
--EXPECTREGEX--
(((PHP )?Warning|Deprecated):\s+Directive 'register_globals' is deprecated in PHP 5\.3 and greater in Unknown on line 0)?(Fatal error: Directive 'register_globals' is no longer available in PHP in Unknown on line 0)?
--DONE--
