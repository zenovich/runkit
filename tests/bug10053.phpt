--TEST--
runkit_method_copy() function
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class runkit_one {
	private $b = 0;
	private $c = 1;

	function runkit_method($naturalNumber) {
		$delta = $this->c + $naturalNumber;
		$thisIsAStrangeVariableWithAVeryLongNameHopeThisWillShowTheError = $delta * $delta;
		$objectVarWithLongName = new stdclass;
		$objectVarWithLongName->e = $thisIsAStrangeVariableWithAVeryLongNameHopeThisWillShowTheError;
		echo "Runkit Method: $naturalNumber\n";
		var_dump($objectVarWithLongName);
	}
}

class runkit_two {
	private $b = 27;
	private $c = 99;

}

$o = new runkit_one();
$o->runkit_method(1);

runkit_method_copy('runkit_two','runkit_method','runkit_one');

$o->runkit_method(2);

$o2 = new runkit_two();
$o2->runkit_method(3);
runkit_method_remove('runkit_one','runkit_method');
if (method_exists('runkit_one','runkit_method')) {
	echo "Runkit Method still exists in Runkit One!\n";
}
$o2->runkit_method(4);
?>
--EXPECT--
Runkit Method: 1
object(stdClass)#2 (1) {
  ["e"]=>
  int(4)
}
Runkit Method: 2
object(stdClass)#2 (1) {
  ["e"]=>
  int(9)
}
Runkit Method: 3
object(stdClass)#3 (1) {
  ["e"]=>
  int(10404)
}
Runkit Method: 4
object(stdClass)#3 (1) {
  ["e"]=>
  int(10609)
}
