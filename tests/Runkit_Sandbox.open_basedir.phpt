--TEST--
Runkit_Sandbox - Prevent overriding open_basedir when a bogus path is present
--FILE--
<?php
ini_set('open_basedir', '/bogus-does-not-exist-runkit-test-dir');
var_dump(ini_get('open_basedir'));

$s = new Runkit_Sandbox(array(
  'open_basedir' => __DIR__,
));

var_dump(__DIR__ === $s->ini_get('open_basedir'));

--EXPECT--
string(37) "/bogus-does-not-exist-runkit-test-dir"
bool(false)
