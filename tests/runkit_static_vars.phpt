--TEST--
Static Variables in runkit modified functions
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
function orig() {
  static $x = 0;
  var_dump(++$x);
}

orig();
runkit_function_copy('orig', 'funccopy');
funccopy();
runkit_function_remove('orig');
funccopy();

echo "====\n";

class C {
  public function orig() {
    static $x = 0;
    var_dump(++$x);
  }
}
$c = new C;

$c->orig();
runkit_method_copy('C', 'copy', 'C', 'orig');
$c->copy();
runkit_method_remove('C', 'orig');
$c->copy();

--EXPECT--
int(1)
int(2)
int(3)
====
int(1)
int(2)
int(3)
