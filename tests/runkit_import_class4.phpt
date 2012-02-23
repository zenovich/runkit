--TEST--
runkit_import() Importing and overriding classes (PHP4)
--SKIPIF--
<?php
    if (PHP_VERSION >= '5.0.0') echo "skip";
    if (!extension_loaded("runkit")) echo "skip"; ?>
--FILE--
<?php
class MyTestClass {
	function foo()
	{
		return "foo()\n";
	}
	function myFoo()
	{
		return "myFoo()\n";
	}
	function myStaticFoo()
	{
		return "myStaticFoo()\n";
	}
}

$obj = new MyTestClass;
print($obj->foo());
print($obj->myFoo());
print(MyTestClass::myStaticFoo());
runkit_import(dirname(__FILE__) . '/runkit_import_class4.inc', RUNKIT_IMPORT_CLASS_METHODS);
print($obj->foo());
print($obj->myFoo());
print(MyTestClass::myStaticFoo());
runkit_import(dirname(__FILE__) . '/runkit_import_class4.inc', RUNKIT_IMPORT_OVERRIDE | RUNKIT_IMPORT_CLASSES);
print($obj->foo());
print($obj->myFoo());
print(MyTestClass::myStaticFoo());
?>
--EXPECTF--
foo()
myFoo()
myStaticFoo()

Notice: runkit_import(): mytestclass::foo() already exists, not importing in %s on line %d

Notice: runkit_import(): mytestclass::myfoo() already exists, not importing in %s on line %d

Notice: runkit_import(): mytestclass::mystaticfoo() already exists, not importing in %s on line %d
foo()
myFoo()
myStaticFoo()
IMPORTED: foo()
IMPORTED: myFoo()
IMPORTED: myStaticFoo()
