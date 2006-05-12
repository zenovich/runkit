--TEST--
Runkit_Sandbox Nesting
--SKIPIF--
<?php if(!extension_loaded("runkit")) print "skip"; 
      /* May not be available due to lack of TSRM interpreter support */
      if(!function_exists("runkit_lint")) print "skip"; ?>
--FILE--
<?php
$php = new Runkit_Sandbox();
$php['output_handler'] = 'child_output';
$php->eval('echo "Foo\n";
			$grandchild = new Runkit_Sandbox();
			$grandchild["output_handler"] = "grandchild_output";
			$grandchild->eval(\'echo "Bar\n";\');
			echo "Baz\n";

			function grandchild_output($str) {
				if (strlen($str) == 0) return NULL; /* flush() */
				return "Grandchild Says: $str";
			}');

echo "Bling\n";
$php->echo("Blong\n");

function child_output($str) {
	if (strlen($str) == 0) return NULL; /* flush() */
	return "Child Says: $str";
}

--EXPECT--
Child Says: Foo
Child Says: Grandchild Says: Bar
Child Says: Baz
Bling
Child Says: Blong
