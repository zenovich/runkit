--TEST--
runkit_import() Importing and overriding method with a static variable
--SKIPIF--
<?php
    if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
?>
--FILE--
<?php
class Test {
    function f() {
        static $v = 0;
        $v++;
        echo $v, "\n";
    }
}

$t = new Test;
$t->f();
runkit_import(dirname(__FILE__) . '/runkit_import_method_with_static_var.inc', RUNKIT_IMPORT_CLASSES | RUNKIT_IMPORT_OVERRIDE);
$t->f();
runkit_import(dirname(__FILE__) . '/runkit_import_method_with_static_var.inc', RUNKIT_IMPORT_CLASSES | RUNKIT_IMPORT_OVERRIDE);
$t->f();
?>
--EXPECT--
1
2
2

