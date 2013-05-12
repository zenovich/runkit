--TEST--
runkit_import() Importing and not overriding subclass properties
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class Test {}
class TestSub extends Test {
    public static $s = "s";
    public $v = "v";
}

$o = new TestSub;
echo $o->v, "\n";
echo TestSub::$s, "\n";
runkit_import(dirname(__FILE__) . '/runkit_import_class_property_not_overriding_subclasses.inc', RUNKIT_IMPORT_CLASS_PROPS | RUNKIT_IMPORT_CLASS_STATIC_PROPS);
$o = new TestSub;
echo $o->v, "\n";
echo TestSub::$s, "\n";
runkit_import(dirname(__FILE__) . '/runkit_import_class_property_not_overriding_subclasses.inc', RUNKIT_IMPORT_CLASS_PROPS | RUNKIT_IMPORT_CLASS_STATIC_PROPS | RUNKIT_IMPORT_OVERRIDE);
echo $o->v, "\n";
$o = new TestSub;
echo $o->v, "\n";
echo TestSub::$s, "\n";
$o->v = 'v';
echo $o->v, "\n";
runkit_import(dirname(__FILE__) . '/runkit_import_class_property_not_overriding_subclasses.inc', RUNKIT_IMPORT_CLASS_PROPS | RUNKIT_IMPORT_CLASS_STATIC_PROPS | RUNKIT_IMPORT_OVERRIDE | RUNKIT_OVERRIDE_OBJECTS);
echo $o->v, "\n";
?>
--EXPECTF--
v
s

Notice: runkit_import(): TestSub::$s already exists, not adding in %s on line %d

Notice: runkit_import(): TestSub->v already exists, not adding in %s on line %d
v
s
v
IMPORTED: v
IMPORTED: s
v
IMPORTED: v
