--TEST--
runkit_class_adopt, runkit_class_emancipate and inheritance
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
class myParent {}
class myUncle {}
class myChild extends myParent {}

echo get_parent_class("myChild"), "\n";

runkit_class_emancipate("myChild");
echo get_parent_class("myChild") . "\n";

runkit_class_adopt("myChild", "myUncle");
echo get_parent_class("myChild") . "\n";

--EXPECTF--
my%sarent

my%sncle
