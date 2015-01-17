--TEST--
runkit_method_redefine() function for protected methods
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--INI--
error_reporting=E_ALL
display_errors=on
--FILE--
<?php
class runkit_class {
	protected $a = 'a';
	protected $b = 'b';
	protected function runkit_method_int($a) {
		static $is = "is";
		echo "{$this->a} $is $a\n";
	}
	public function runkit_method($a) {
		return $this->runkit_method_int($a);
	}
}
$obj = new runkit_class();
$obj->runkit_method('foo');
runkit_method_copy('runkit_class','runkit_method_old','runkit_class','runkit_method_int');
runkit_method_redefine('runkit_class','runkit_method_int','$b', 'static $is="is"; echo "{$this->b} $is $b\n";');
$obj->runkit_method('bar');
runkit_method_remove('runkit_class','runkit_method_int');
runkit_method_copy('runkit_class','runkit_method_int','runkit_class','runkit_method_old');
runkit_method_remove('runkit_class','runkit_method_old');
$obj1 = new runkit_class();
$obj1->runkit_method('foo');
?>
--EXPECT--
a is foo
b is bar
a is foo
