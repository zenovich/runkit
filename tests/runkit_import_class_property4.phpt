--TEST--
runkit_import() Importing and overriding class properties
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip";
      if(PHP_VERSION >= '5.0.0') print "skip";
?>
--FILE--
<?php
class Test {
    var $v = "v";
}

$o = new Test;
echo $o->v, "\n";
runkit_import(dirname(__FILE__) . '/runkit_import_class_property4.inc', RUNKIT_IMPORT_CLASS_PROPS);
$o = new Test;
echo $o->v, "\n";
runkit_import(dirname(__FILE__) . '/runkit_import_class_property4.inc', RUNKIT_IMPORT_CLASS_PROPS | RUNKIT_IMPORT_CLASS_STATIC_PROPS | RUNKIT_IMPORT_OVERRIDE);
$o = new Test;
echo $o->v, "\n";
?>
--EXPECTF--
v

Notice: runkit_import(): test->v already exists, not importing in %s on line %d
v
IMPORTED: v
