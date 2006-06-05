--TEST--
runkit_function_copy() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function runkit_sample($n) {
  echo "Runkit Sample: $n\n";
}

runkit_sample(1);
runkit_function_copy('runkit_sample','runkit_duplicate');
runkit_sample(2);
runkit_duplicate(3);
runkit_function_remove('runkit_sample');
if (function_exists('runkit_sample')) {
	echo "runkit_sample() still exists!\n";
}
runkit_duplicate(4);
?>
--EXPECT--
Runkit Sample: 1
Runkit Sample: 2
Runkit Sample: 3
Runkit Sample: 4
