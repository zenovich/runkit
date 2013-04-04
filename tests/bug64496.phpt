--TEST--
Bug #64496 - Runkit_Sandbox override of open_basedir when parent uses multiple paths
--FILE--
<?php
$tmp = realpath(sys_get_temp_dir());
$dir = realpath(__DIR__);
$parent = realpath(dirname(dirname(__DIR__)));
ini_set('open_basedir', sys_get_temp_dir() . ':' . dirname(__DIR__));

foreach(array(
  "$tmp:$dir",
  "$dir:$tmp",
  $dir,
  $tmp,
  '/bogus-does-not-exist-runkit-test-dir',
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
