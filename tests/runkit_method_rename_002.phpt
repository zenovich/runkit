--TEST--
Rename children of ancestor methods
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
function getClassMethods($class)
{
    $methods = array();
    $class = new ReflectionClass($class);
    foreach ($class->getMethods() as $method) {
        array_push($methods, $method->getName());
    }
    return $methods;
}

class Ancestor
{
    public function __construct(){}
}

class Descendant extends Ancestor
{
    public function someMethod(){}
}

class AnotherDescendant extends Ancestor
{
    public function anotherMethod(){}
}

runkit_method_rename('Ancestor', '__construct', 'abcdefghmnoprst');
print_r(getClassMethods('Descendant'));
print_r(getClassMethods('AnotherDescendant'));
--EXPECT--
Array
(
    [0] => someMethod
    [1] => abcdefghmnoprst
)
Array
(
    [0] => anotherMethod
    [1] => abcdefghmnoprst
)
