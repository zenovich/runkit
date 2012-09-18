--TEST--
Test for caching issues on redefining class methods
--SKIPIF--
<?php
if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class RunkitClass {
  function a() {
  }
}

class Test {
  var $obj;

  function a($result) {
    for ($i = 0; $i < 10; $i++) {
      runkit_method_redefine('RunkitClass', 'a', '', "echo ''; return $result + 1;");
      $result = $this->obj->a($result);
    }
    return $result;
  }

  function b($result) {
    for ($i = 0; $i < 10; $i++) {
      runkit_method_remove('RunkitClass', 'a');
      runkit_method_add('RunkitClass', 'a', '', "echo ''; return $result + 1;");
      $result = $this->obj->a();
    }
    return $result;
  }

  function c($result) {
    for ($i = 0; $i < 10; $i++) {
      runkit_import('runkit_methods_redefining_and_cache.inc', RUNKIT_IMPORT_CLASSES | RUNKIT_IMPORT_OVERRIDE);
      $result = $this->obj->a($result);
    }
    return $result;
  }

  function t() {
    $this->obj = new RunkitClass();
    $result = 0;
    for ($i = 0; $i < 10; $i++) {
      $result = $this->a($result);
    }
    echo $result, "\n";

    for ($i=0; $i<10; $i++) {
      $result = $this->b($result);
    }
    echo $result, "\n";

    for ($i=0; $i<10; $i++) {
      $result = $this->c($result);
    }
    echo $result, "\n";
  }
}

$t = new Test();
$t->t();

$obj = new RunkitClass();
function a($result, $obj) {
  for ($i = 0; $i < 10; $i++) {
    runkit_method_redefine('RunkitClass', 'a', '', "echo ''; return $result + 1;");
    $result = $obj->a();
  }
  return $result;
}
$result = 300;
for ($i = 0; $i < 10; $i++) {
  $result = a($result, $obj);
}
echo $result, "\n";

function b($result, $obj) {
  for ($i = 0; $i < 10; $i++) {
    runkit_method_redefine('RunkitClass', 'a', '', "echo ''; return $result + 1;");
    $result = $obj->a();
  }
  return $result;
}
for ($i = 0; $i < 10; $i++) {
  $result = b($result, $obj);
}
echo $result, "\n";
--EXPECT--
100
200
300
400
500
