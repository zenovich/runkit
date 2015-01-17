--TEST--
Bug#56976 - Failure adding __call method
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; ?>
--FILE--
<?php
class ParentClass
{
    public function foo()
    {
        echo "ParentClass::foo\n";
    }
}

class ChildClass extends ParentClass { }

var_dump(runkit_method_add('ParentClass', '__call', '$method, $args',
                           'echo "In ParentClass::__call()\n";' .
                           'call_user_func_array(array($this, "prefix_{$method}"), $args);'));

var_dump(runkit_method_rename('ChildClass', 'foo', 'prefix_foo'));

$o = new ChildClass;
$o->foo();

--EXPECT--
bool(true)
bool(true)
In ParentClass::__call()
ParentClass::foo
