<?php
class Foo extends Bar {
	public function a() { print "IMPORTED: Hello World!\n"; }
}

class Bar {
	public function b() { print "IMPORTED: Hello World from Bar!\n"; }
}
?>
