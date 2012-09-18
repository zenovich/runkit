--TEST--
Test for caching issues on redefining functions
--SKIPIF--
<?php
if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
runkit.internal_override=On
--FILE--
<?php
function a($result) {
  for ($i = 0; $i < 10; $i++) {
    runkit_function_redefine('sprintf', '$v', "echo ''; return $result + 1;");
    $result = sprintf('a');
  }
  return $result;
}

$result = 0;
for ($i = 0; $i < 10; $i++) {
  $result = a($result);
}

echo $result, "\n";

function b($result) {
  for ($i = 0; $i < 10; $i++) {
    runkit_function_remove('sprintf');
    runkit_function_add('sprintf', '$v', "echo ''; return $result + 1;");
    $result = sprintf('a');
  }
  return $result;
}

for ($i=0; $i<10; $i++) {
  $result = b($result);
}
echo $result, "\n";

class A {
  function run() {
    echo '';
    return mail('a');
  }

  function c($result) {
    for ($i = 0; $i < 10; $i++) {
      runkit_function_redefine('mail', '$v', "echo ''; return $result + 1;");
      $result = $this->run();
    }
    return $result;
  }

  function d($result) {
    for ($i = 0; $i < 10; $i++) {
      runkit_function_remove('mail');
      runkit_function_add('mail', '$v', "echo ''; return $result + 1;");
      $result = $this->run();
    }
    return $result;
  }
}
$a = new A();
for ($i = 0; $i < 10; $i++) {
  $result = $a->c($result);
}
echo $result, "\n";
for ($i = 0; $i < 10; $i++) {
  $result = $a->d($result);
}
echo $result, "\n";

function e($result) {
    for ($i = 0; $i < 10; $i++) {
      runkit_import('runkit_functions_redefining_and_cache.inc', RUNKIT_IMPORT_FUNCTIONS | RUNKIT_IMPORT_OVERRIDE);
      $result = sprintf($result);
    }
    return $result;
}

for ($i = 0; $i < 10; $i++) {
  $result = e($result);
}
echo $result;
--EXPECT--
100
200
300
400
500
