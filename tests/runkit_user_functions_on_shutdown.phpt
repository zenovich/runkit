--TEST--
user-defined functions should remain after runkit's shutdown
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION || !function_exists('session_start')) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=On
runkit.internal_override=On
--FILE--
<?php
header('Content-Type: text/plain');

function sleepPrepare()
{
    print_r(1);
    return array('property');
}

class RunkitTest
{
    public $property;

    public function __sleep()
    {
        return sleepPrepare();
    }
}

runkit_function_redefine('print_r', '$v', 'echo "OK";');
session_start();

$o = new RunkitTest();
$o->property = 'testvalue';

$_SESSION['runkittest'] = $o;
--EXPECTF--
OK
