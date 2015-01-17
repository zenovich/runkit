--TEST--
Bug#57658 - runkit_class_adopt fails on method names with capitals
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL & ~E_STRICT);

class A { function aB() { print "a";} function aC() { echo "d"; } }
class B { function aB() { print "b";} }
runkit_class_adopt("B", "A");
B::aC();
--EXPECT--
d
