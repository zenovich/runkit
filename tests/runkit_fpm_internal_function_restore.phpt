--TEST--
Test restoring internal functions after renaming and copying under fpm
--SKIPIF--
<?php include "_fpm_skipif.inc"; ?>
--FILE--
<?php
include "_fpm_include.inc";
$code = <<<EOT
<?php
echo "Test Start\n";
runkit_function_copy('chop', '__chop');
runkit_function_rename('chop', '_chop');
echo _chop('A B '), "\n";
echo __chop('C D '), "\n";
echo "Test End\n";
EOT;
fpm_test(array($code, $code, $code), "-d extension_dir=modules/ -d extension=runkit.so -d runkit.internal_override=1");
?>
Done
--EXPECTF--
[%s] NOTICE: fpm is running, pid %d
[%s] NOTICE: ready to handle connections
Test Start
A B
C D
Test End

Request ok
Test Start
A B
C D
Test End

Request ok
Test Start
A B
C D
Test End

Request ok
[%s] NOTICE: Terminating ...
[%s] NOTICE: exiting, bye-bye!
Done
--CLEAN--
<?php
include "_fpm_include.inc";
fpm_clean();
?>
