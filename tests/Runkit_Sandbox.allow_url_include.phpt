--TEST--
Runkit_Sandbox - Allow disabling of allow_url_include
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; ?>
--INI--
allow_url_include="On"
--FILE--
<?php
var_dump(ini_get('allow_url_include'));

$s = new Runkit_Sandbox(array(
  'allow_url_include' => false,
));

var_dump($s->ini_get('allow_url_include'));

--EXPECT--
string(2) "On"
string(1) "0"
