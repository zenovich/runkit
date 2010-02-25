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

	function runkitMethod($n) {
		echo "Runkit Method: $n\n";
	}
}

class runkit_two {
}

runkit_one::runkit_method(1);
runkit_method_copy('runkit_two','runkit_method','runkit_one');
runkit_method_copy('runkit_two','runkitMethod','runkit_one');
runkit_one::runkit_method(2);
runkit_two::runkit_method(3);
runkit_one::runkitMethod(4);
runkit_two::runkitmethod(5);
runkit_two::runkitMethod(6);
runkit_method_remove('runkit_one','runkit_method');
if (method_exists('runkit_one','runkit_method')) {
	echo "runkit_method still exists in Runkit One!\n";
}
runkit_method_remove('runkit_one','runkitMethod');
if (method_exists('runkit_one','runkitMethod')) {
	echo "runkitMethod still exists in Runkit One!\n";
}
runkit_two::runkit_method(7);
runkit_two::runkitMethod(8);
$reflMethod = new ReflectionMethod('runkit_two', 'runkitMethod');
echo $reflMethod->getDeclaringClass()->getName(), "\n";
echo $reflMethod->getName(), "\n";
?>
--EXPECT--
Runkit Method: 1
Runkit Method: 2
Runkit Method: 3
Runkit Method: 4
Runkit Method: 5
Runkit Method: 6
Runkit Method: 7
Runkit Method: 8
runkit_two
runkitMethod
