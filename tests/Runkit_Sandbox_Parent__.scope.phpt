--TEST--
Runkit_Sandbox_Parent Class -- Scopes
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_sandbox_output_handler")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$php['parent_access'] = true;
$php['parent_read'] = true;
$php->ini_set('display_errors', true);
$php->ini_set('html_errors', false);

$test = "Global";

$php->eval('$PARENT = new Runkit_Sandbox_Parent;');

$php['parent_scope'] = 0;
one();

$php['parent_scope'] = 1;
one();

$php['parent_scope'] = 2;
one();

$php['parent_scope'] = 3;
one();

$php['parent_scope'] = 4;
one();

$php['parent_scope'] = 5;
one();

function one() {
	$test = "one()";
	two();
}

function two() {
	$test = "two()";
	three();
}

function three() {
	$test = "three()";
	$GLOBALS['php']->eval('var_dump($PARENT->test);');
}
--EXPECT--
string(6) "Global"
string(7) "three()"
string(5) "two()"
string(5) "one()"
string(6) "Global"
string(6) "Global"
