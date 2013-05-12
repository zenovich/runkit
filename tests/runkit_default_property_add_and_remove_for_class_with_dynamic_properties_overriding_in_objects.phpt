--TEST--
runkit_default_property_add() and runkit_default_property_remove() functions on classes having dynamic properties overriding in objects
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode('.', PHP_VERSION)) < 5) print "skip";
?>
--FILE--
<?php
class A {
	var $d=3;
}

class B extends A {
}

$o = new A;
$o->b = 1;
echo 'b=', $o->b, "\n";
echo 'd=', $o->d, "\n";
runkit_default_property_add('A', 'b', 2, RUNKIT_OVERRIDE_OBJECTS);
echo 'b=', $o->b, "\n";
echo 'd=', $o->d, "\n";
runkit_default_property_remove('A', 'b');
$o1 = new A;
echo 'b=', $o->b, "\n";
echo 'b=', @$o1->b, "\n";
echo 'd=', $o->d, "\n";
echo 'd=', $o1->d, "\n";

echo "\n";

$o = new B;
echo 'b=', @$o->b, "\n";
echo 'd=', $o->d, "\n";
$o->b = 1;
echo 'b=', $o->b, "\n";
echo 'd=', $o->d, "\n";
runkit_default_property_add('A', 'b', 2, RUNKIT_OVERRIDE_OBJECTS);
echo 'b=', $o->b, "\n";
echo 'd=', $o->d, "\n";
runkit_default_property_remove('A', 'b');
$o1 = new B;
echo 'b=', $o->b, "\n";
echo 'b=', @$o1->b, "\n";
echo 'd=', $o->d, "\n";
echo 'd=', $o1->d, "\n";
runkit_default_property_remove('A', 'd', 1);
$o1 = new B;
echo 'd=', @$o->d, "\n";
echo 'd=', @$o1->d, "\n";
--EXPECT--
b=1
d=3
b=2
d=3
b=2
b=
d=3
d=3

b=
d=3
b=1
d=3
b=2
d=3
b=2
b=
d=3
d=3
d=
d=
