<?php
return new Test;

class Test {
    function f() {
        static $v = 1;
        $v++;
        echo $v, "\n";
    }
}
