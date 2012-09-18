--TEST--
runkit_method_redefine() must also update children methods' prototypes 
--FILE--
<?php
class a
{
        protected function foo()
        {
        }

        public function test()
        {
                $this->foo();
        }
}

class b extends a {

        protected function foo()
        {
        }
}

class c extends b {
        function bar()
        {
                $this->test();
        }
}

runkit_method_redefine('a', 'foo', '', 'var_dump("new foo()");');

$c = new c;
$c->bar();

echo "==DONE==\n";

?>
--EXPECT--
==DONE==
