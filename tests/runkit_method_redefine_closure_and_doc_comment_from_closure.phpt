--TEST--
runkit_method_redefine() function with closure and doc_comment from closure
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--INI--
display_errors=on
--FILE--
<?php
class runkit_class {
	public function runkit_method() {}
}
runkit_method_redefine('runkit_class','runkit_method', /** new doc_comment */ function() {});
$r1 = new ReflectionMethod('runkit_class', 'runkit_method');
echo $r1->getDocComment(), "\n";
?>
--EXPECT--
/** new doc_comment */
