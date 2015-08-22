--TEST--
runkit_class_adopt() function and properties issues
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip"; ?>
--FILE--
<?php
class C {
	public function to_be_mocked() {
		return 1;
	}
}

class A {
	private $t = 1; //private var
	public function to_be_mocked() {
		//this method is executed in the context of B, so the properties are looked up there
		var_dump($this->t);
		return 1;
	}
}

class B extends C {
	public function foo() {
		return 1 + $this->to_be_mocked();
	}
}
$r = new B();
runkit_class_emancipate('B');
runkit_class_adopt('B', "A");
$o = new B();
$o->foo();
$r->foo();
runkit_class_emancipate('B');
$s = new B();
print_r($o);
print_r($r);
print_r($s);
?>
--EXPECTF--
int(1)
int(1)
B Object
(
)
B Object
(
)
B Object
(
)
