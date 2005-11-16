--TEST--
runkit_return_value_used() function
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; 
?>
--FILE--
<?php
function two() {
  echo 'two()\'s return value used: ';
  var_dump(runkit_return_value_used());
}

function one() {
  echo 'one()\'s return value used: ';
  var_dump(runkit_return_value_used());
  two();
  $t = two();
}

one();
$o = one();

echo 'main()\'s return value used: ';
var_dump(runkit_return_value_used());
?>
--EXPECTF--
one()'s return value used: bool(false)
two()'s return value used: bool(false)
two()'s return value used: bool(true)
one()'s return value used: bool(true)
two()'s return value used: bool(false)
two()'s return value used: bool(true)
main()'s return value used: bool(false)
