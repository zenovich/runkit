--TEST--
runkit_import() Importing and overriding classes extending another loaded class
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class ext{
	function ver(){
		print "n\n";
	}
}

runkit_import(dirname(__FILE__) . '/runkit_import_class_extend.inc', RUNKIT_IMPORT_CLASSES | RUNKIT_IMPORT_OVERRIDE);
$Test = new Test;
$Test->ver();
unset($Test);
//load it once more to override
runkit_import(dirname(__FILE__) . '/runkit_import_class_extend.inc', RUNKIT_IMPORT_CLASSES | RUNKIT_IMPORT_OVERRIDE);
$Test = new Test;
$Test->ver();
$Test->aaa();
--EXPECTF--
n
n
n

