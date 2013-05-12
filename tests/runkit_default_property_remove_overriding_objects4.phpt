--TEST--
runkit_default_property_remove() function with overriding objects (PHP4)
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) != 4) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=On
--FILE--
<?php
class RunkitClass {
    var $publicProperty = 1;
}

ini_set('display_errors', 1);
ini_set('error_reporting', E_ALL);

$className = 'RunkitClass';
$obj = new $className();
runkit_default_property_remove($className, 'publicProperty', TRUE);
print_r($obj);

?>
--EXPECTF--
Warning: runkit_default_property_remove(): Removing from objects is not supported for PHP versions below 5.0 in %s on line %d
runkitclass Object
(
    [publicProperty] => 1
)
