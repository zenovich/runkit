<?php

class MyTestClass {
    public static $v = "IMPORTED: v\n";

    function foo()
    {
	    return "IMPORTED: foo()\n";
    }

    function myFoo()
    {
	    return "IMPORTED: myFoo()\n";
    }

    static function myStaticFoo()
    {
	    return "IMPORTED: myStaticFoo()\n";
    }
}

?>
