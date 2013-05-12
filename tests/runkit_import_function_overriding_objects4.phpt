--TEST--
runkit_import() Importing and overriding objects (PHP4)
--SKIPIF--
<?php
    if (PHP_VERSION >= '5.0.0') echo "skip";
    if (!extension_loaded("runkit")) echo "skip"; ?>
--FILE--
<?php
class MyTestClass {
}

$obj = new MyTestClass;
runkit_import(dirname(__FILE__) . '/runkit_import_function_overriding_objects4.inc', RUNKIT_IMPORT_CLASS_METHODS | RUNKIT_OVERRIDE_OBJECTS);
var_dump($obj);
?>
--EXPECTF--
Warning: runkit_import(): Overriding in objects is not supported for PHP versions below 5.0 in %s on line %d
object(mytestclass)(0) {
}
