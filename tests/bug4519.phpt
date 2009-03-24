--TEST--
Bug #4519 Unable to override class definitions of a derived class
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
class Foo extends Bar {
        public function a() { print "Hello World!\n"; }
}

class Bar {
        public function b() { print "Hello World from Bar!\n"; }
}


$test = new Foo();
print($test->b());
runkit_import(dirname(__FILE__) . '/bug4519_inc.php', RUNKIT_IMPORT_OVERRIDE | RUNKIT_IMPORT_CLASSES);
$test2 = new Foo();
print($test2->b());
?>
--EXPECT--
Hello World from Bar!
IMPORTED: Hello World from Bar!
