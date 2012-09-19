--TEST--
runkit_import() Importing non-existent file
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
runkit_import('non-existent_file.unknown');
echo "After error";
?>
--EXPECTF--
Warning: runkit_import(non-existent_file.unknown): failed to open stream: No such file or directory in %s on line %d

Warning: runkit_import(): Failed opening 'non-existent_file.unknown' for inclusion %s on line %d

Warning: runkit_import(): Import Failure in %s on line %d
After error
