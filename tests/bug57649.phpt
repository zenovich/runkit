--TEST--
Bug#57649 - runkit_import() - methods not added - multiple classes in one file
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip";
      if(version_compare(PHP_VERSION, '5.0.0', '<')) print "skip";
?>
--FILE--
<?php
class b {
  public function foobar() {
    echo "foobar()\n";
  }
}

class a extends b { }

runkit_import( dirname(__FILE__) . "/bug57649.inc" );

$a = new a();
$a->foobar();
$a->foo();
var_dump(class_exists('c'));
--EXPECT--
foobar()
foo()
bool(true)
