--TEST--
runkit_import() Importing file with syntax error in it
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
runkit_import(dirname(__FILE__) . '/runkit_import_with_syntax_error.inc');
echo "After syntax error";
?>
--EXPECTF--
Parse error: syntax error, unexpected ')' in /home/dzenovich/runkit-my/tests/runkit_import_with_syntax_error.inc on line 2
