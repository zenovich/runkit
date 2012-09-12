--TEST--
add old-style parent ctor
--FILE--
<?php

class Test {
}

class FOO_test extends test {
}

runkit_method_add("test", "test", "", "var_dump('new constructor');");
$a = new foo_test;

echo "==DONE==\n";
?>
--EXPECT--
string(15) "new constructor"
==DONE==
