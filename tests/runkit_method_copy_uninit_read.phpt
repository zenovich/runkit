--TEST--
runkit_method_copy() causes uninitialized read 
--FILE--
<?php

runkit_method_copy('test', "new_method", "test", "old_method");

class test
{
    function old_method() {
            if(1)
                $this->a[] = "b";
    }
}

echo "==DONE==\n";

?>
--EXPECT--
==DONE==
