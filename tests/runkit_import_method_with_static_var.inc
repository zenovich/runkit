<?php
return new Test;

class Test {
    public function f() {
        static $v = 1;
        $v++;
        echo $v, "\n";
    }
}
