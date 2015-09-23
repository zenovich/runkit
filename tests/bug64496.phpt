--TEST--
Bug #64496 - Runkit_Sandbox override of open_basedir when parent uses multiple paths
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_SANDBOX) print "skip";
      if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--FILE--
<?php
$tmp = realpath(sys_get_temp_dir());
$dir = realpath(dirname(__FILE__));
$parent = realpath(dirname(dirname(dirname(__FILE__))));
ini_set('open_basedir', sys_get_temp_dir() . PATH_SEPARATOR . dirname(dirname(__FILE__)));

foreach(array(
  $tmp . PATH_SEPARATOR . $dir,
  $dir . PATH_SEPARATOR . $tmp,
  $dir,
  $tmp,
  DIRECTORY_SEPARATOR . 'bogus-does-not-exist-runkit-test-dir',
  $parent,
) as $idx => $path) {
  $s = new Runkit_Sandbox(array(
    'open_basedir' =>  $path
  ));

  echo "$idx: ";
  var_dump($path === $s->ini_get('open_basedir'));
}
--EXPECT--
0: bool(true)
1: bool(true)
2: bool(true)
3: bool(true)
4: bool(false)
5: bool(false)
