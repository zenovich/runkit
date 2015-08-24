--TEST--
adding magic serialize method to common class should be ignored
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class Test {
}

$a = new Test();
runkit_method_add("Test", "serialize", '', 'echo "serialize\n";');
runkit_method_add("Test", "unserialize", '$s', 'echo "unserialize\n";', RUNKIT_ACC_STATIC);
$s1 = serialize($a);
$b = unserialize($s1);
echo "step2\n";
$a->serialize();
Test::unserialize($s1);
?>
--EXPECT--
step2
serialize
unserialize
