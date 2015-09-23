--TEST--
Bug#56662 - Wrong access level with RUNKIT_ACC_PUBLIC
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
class A {}
runkit_method_add ('A', 'x', '', '', RUNKIT_ACC_PUBLIC);
Reflection::export(new ReflectionMethod('A', 'x'));

class B extends A { public function x() {} }
Reflection::export(new ReflectionMethod('B', 'x'));

eval("class C extends A { public function x() {} }");
Reflection::export(new ReflectionMethod('C', 'x'));

--EXPECTF--
Method [ <user%S> public method x ] {
  @@ %sbug56662.php(3) : runkit runtime-created function 1 - 1
}

Method [ <user, overwrites A%S> public method x ] {
  @@ %sbug56662.php 6 - 6
}

Method [ <user, overwrites A, prototype A> public method x ] {
  @@ %sbug56662.php(9) : eval()'d code 1 - 1
}
