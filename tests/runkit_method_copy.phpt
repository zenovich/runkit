--TEST--
runkit_method_copy() function
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class runkit_one {
	function runkit_method($n) {
		echo "Runkit Method: $n\n";
	}
}

class runkit_two {
}

runkit_one::runkit_method(1);
runkit_method_copy('runkit_two','runkit_method','runkit_one');
runkit_one::runkit_method(2);
runkit_two::runkit_method(3);
runkit_method_remove('runkit_one','runkit_method');
if (method_exists('runkit_one','runkit_method')) {
	echo "Rukit Method still exists in Runkit One!\n";
}
runkit_two::runkit_method(4);
?>
--EXPECT--
Runkit Method: 1
Runkit Method: 2
Runkit Method: 3
Runkit Method: 4

