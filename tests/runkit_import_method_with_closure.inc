<?php
return new Test;

class Test {
    public function F() { // Uppercase should override anyway
        $f = function() {
            echo "IMPORTED: f\n";
        };
        $f();
    }
}

