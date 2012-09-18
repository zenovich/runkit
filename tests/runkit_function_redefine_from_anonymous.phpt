--TEST--
runkit_function_redefine() and call from anonymous function
--SKIPIF--
<?php
	if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
	if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=on
runkit.internal_override=On
--FILE--
<?php
function greet(){
	echo "hey\n";
}
greet();
runkit_function_redefine("greet",'$name',"echo \"hey \$name\n\";");
greet("you");
function parent() {
	$localvar = "john";
	$af = function() use ($localvar){
		greet($localvar);
	};
	return $af;
}
$greet1 = parent();
$greet1();
runkit_function_redefine("greet",'$name',"echo \"hello \$name\n\";");
$greet1();
--EXPECT--
hey
hey you
hey john
hello john

