--TEST--
Runkit_Sandbox with register_globlas
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; ?>
--INI--
register_globals=On
--FILE--
<?php
$php = new Runkit_Sandbox();
--EXPECTREGEX--
(((PHP )?Warning|Deprecated):\s+Directive 'register_globals' is deprecated in PHP 5\.3 and greater in Unknown on line 0)?
--DONE--
