--TEST--
runkit_method_rename() function should set method's scope correctly
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
display_errors=on
--FILE--
<?php
class Test
{
    protected function func1()
    {
	echo "func1\n";
    }
}

runkit_method_add('Test', 'func2', '', 'echo "func2\n"; $this->func1();', RUNKIT_ACC_PUBLIC);
$a = new Test();
$a->func2();

runkit_method_rename('Test', 'func1', '_func1');
runkit_method_redefine('Test', 'func2', '', 'echo "new func2\n"; $this->_func1();', RUNKIT_ACC_PUBLIC);
$b = new Test();
$b->func2();
$a->func2();
?>
--EXPECT--
func2
func1
new func2
func1
new func2
func1
