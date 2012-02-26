--TEST--
runkit_import() Importing and overriding function with a static variable
--SKIPIF--
<?php
    if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
?>
--FILE--
<?php
function f() {
	static $v = 0;
	$v++;
	echo $v, "\n";
}

f();
runkit_import(dirname(__FILE__) . '/runkit_import_function_with_static_var.inc', RUNKIT_IMPORT_FUNCTIONS | RUNKIT_IMPORT_OVERRIDE);
f();
runkit_import(dirname(__FILE__) . '/runkit_import_function_with_static_var.inc', RUNKIT_IMPORT_FUNCTIONS | RUNKIT_IMPORT_OVERRIDE);
f();
?>
--EXPECT--
1
2
2

