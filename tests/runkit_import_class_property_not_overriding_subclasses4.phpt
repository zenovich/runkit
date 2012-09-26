--TEST--
runkit_import() Importing and not overriding subclass properties (PHP4)
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(PHP_VERSION >= '5.0.0') print "skip";
?>
--FILE--
<?php
class Test {}
class TestSub extends Test {
    var $v = "v";
}

$o = new TestSub;
echo $o->v, "\n";
runkit_import(dirname(__FILE__) . '/runkit_import_class_property_not_overriding_subclasses4.inc', RUNKIT_IMPORT_CLASS_PROPS);
$o = new TestSub;
echo $o->v, "\n";
runkit_import(dirname(__FILE__) . '/runkit_import_class_property_not_overriding_subclasses4.inc', RUNKIT_IMPORT_CLASS_PROPS | RUNKIT_IMPORT_OVERRIDE);
$o = new TestSub;
echo $o->v, "\n";
?>
--EXPECTF--
v

Notice: runkit_import(): testsub->v already exists, not adding in %s on line %d
v
IMPORTED: v
