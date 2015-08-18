--TEST--
user-defined functions should remain after runkit's shutdown
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=On
--FILE--
<?php
header('Content-Type: text/plain');

function sleepPrepare()
{
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

session_start();

$o = new RunkitTest();
$o->property = 'testvalue';

$_SESSION['runkittest'] = $o;
echo 'OK';
--EXPECTF--
OK
