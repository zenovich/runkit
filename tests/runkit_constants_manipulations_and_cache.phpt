--TEST--
Test for caching issues on manipulations with constants
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
define('A', 0);
class RunkitClass {
    const A = 0;
}

class Test {
  function a($result) {
    for ($i = 0; $i < 10; $i++) {
      runkit_constant_redefine('RunkitClass::A', RunkitClass::A+1);
      $result = RunkitClass::A;
      runkit_constant_remove('RunkitClass::A');
      runkit_constant_add('RunkitClass::A', $result+1);
      runkit_constant_redefine('A', A+1);
      $result = A;
      runkit_constant_remove('A');
      runkit_constant_add('A', $result+1);
    }
    return A;
  }

  function t() {
    $result = 0;
    for ($i = 0; $i < 10; $i++) {
      $result = $this->a($result);
    }
    echo "" . A . "\n";
    echo "" . RunkitClass::A . "\n";
  }
}

$t = new Test();
$t->t();
--EXPECT--
200
200
