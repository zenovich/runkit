--TEST--
adding and removing magic methods
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php

class Test {
}

class FOO_test extends test {
}

class FOO_test_Child extends FOO_test {
}

runkit_method_add("Test", "__construct", "", 'echo "__construct\n";');
runkit_method_add("Test", "__destruct", "", 'echo "__destruct\n";');
runkit_method_add("Test", "__get", "", 'echo "__get\n";');
runkit_method_add("Test", "__set", "", 'echo "__set\n";');
runkit_method_add("Test", "__call", "", 'echo "__call\n";');
runkit_method_add("Test", "__unset", "", 'echo "__unset\n";');
runkit_method_add("Test", "__isset", "", 'echo "__isset\n";');
runkit_method_add("Test", "__callStatic", "", 'echo "__callstatic\n";', RUNKIT_ACC_STATIC);
$a = new test;
$b = new foo_test;
$c = new FOO_test_Child;
$a->test;
$b->test;
$c->test;
$a->test = 1;
$b->test = 2;
$c->test = 3;
$a->method();
$b->method();
$c->method();
unset($a->test);
unset($b->test);
unset($c->test);
isset($a->test);
isset($b->test);
isset($c->test);
$a = NULL;
$b = NULL;
$c = NULL;
if(version_compare(PHP_VERSION, '5.2.999', '>')) {
	Test::method();
	FOO_Test::method();
	FOO_Test_child::method();
} else {
	echo "__callstatic\n";
	echo "__callstatic\n";
	echo "__callstatic\n";
}

runkit_method_remove("Test", "__construct");
runkit_method_remove("Test", "__destruct");
runkit_method_remove("Test", "__get");
runkit_method_remove("Test", "__set");
runkit_method_remove("Test", "__call");
runkit_method_remove("Test", "__unset");
runkit_method_remove("Test", "__isset");
runkit_method_remove("Test", "__callstatic");
echo "after removing\n";

$a = new test;
$b = new foo_test;
$c = new FOO_test_Child;
$a->test;
$b->test;
$c->test;
$a->test = 1;
$b->test = 2;
$c->test = 3;
$a->method();
$b->method();
$c->method();
unset($a->test);
unset($b->test);
unset($c->test);
isset($a->test);
isset($b->test);
isset($c->test);
$a = NULL;
$b = NULL;
$c = NULL;
FOO_Test_child::method();
?>
--EXPECTF--
__construct
__construct
__construct
__get
__get
__get
__set
__set
__set
__call
__call
__call
__unset
__unset
__unset
__isset
__isset
__isset
__destruct
__destruct
__destruct
__callstatic
__callstatic
__callstatic
after removing

Notice: Undefined property: %s in %s on line %d

Notice: Undefined property: %s in %s on line %d

Notice: Undefined property: %s in %s on line %d

Fatal error: Call to undefined method %s in %s on line %d
