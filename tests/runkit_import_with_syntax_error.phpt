--TEST--
runkit_import() Importing file with syntax error in it
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
runkit_import(dirname(__FILE__) . '/runkit_import_with_syntax_error.inc');
echo "After syntax error";
?>
--EXPECTF--
Parse error: %s in %s on line 2
