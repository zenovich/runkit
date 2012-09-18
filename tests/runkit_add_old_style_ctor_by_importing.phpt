--TEST--
add old-style parent ctor by importing
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php

class Test {
}

class FOO_test extends test {
}

class FOO_test_Child extends FOO_test {
}

runkit_import("runkit_add_old_style_ctor_by_importing.inc", RUNKIT_IMPORT_CLASSES);
$a = new test;
$a = new foo_test;
$a = new FOO_test_Child;

echo "==DONE==\n";
?>
--EXPECT--
string(15) "new constructor"
string(15) "new constructor"
string(15) "new constructor"
==DONE==
