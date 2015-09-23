--TEST--
runkit_import() Importing and overriding class methods
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php

error_reporting(E_ALL & ~E_STRICT);

class ParentClass {
  function foo() {
    echo "Parent::foo\n";
  }
}

include dirname(__FILE__) . '/runkit_import_methods1.inc';

ParentClass::foo();
Child::foo();

runkit_import(dirname(__FILE__) . '/runkit_import_methods2.inc', RUNKIT_IMPORT_CLASS_METHODS);
Child::foo();

runkit_import(dirname(__FILE__) . '/runkit_import_methods2.inc', RUNKIT_IMPORT_CLASS_METHODS | RUNKIT_IMPORT_OVERRIDE);
Child::foo();

--EXPECTF--
Parent::foo
Child1::foo

Notice: runkit_import(): %child::foo() already exists, not importing in %srunkit_import_methods.php on line %d
Child1::foo
Child2::foo
