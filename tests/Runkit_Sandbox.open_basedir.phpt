--TEST--
Runkit_Sandbox - Prevent overriding open_basedir when a bogus path is present
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; ?>
--INI--
open_basedir="/bogus-does-not-exist-runkit-test-dir"
--FILE--
<?php
//ini_set('open_basedir', '/bogus-does-not-exist-runkit-test-dir');
var_dump(ini_get('open_basedir'));

$s = new Runkit_Sandbox(array(
  'open_basedir' => dirname(__FILE__),
));

var_dump(dirname(__FILE__) === $s->ini_get('open_basedir'));

--EXPECT--
string(37) "/bogus-does-not-exist-runkit-test-dir"
bool(false)
