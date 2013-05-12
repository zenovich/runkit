--TEST--
runkit_default_property_remove() remove private properties with inheritance overriding objects
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      if(array_shift(explode(".", PHP_VERSION)) < 5) print "skip";
?>
--INI--
error_reporting=E_ALL
display_errors=On
--FILE--
<?php
class RunkitClass {
    private $privateProperty = "original";
    function getPrivate() {return $this->privateProperty;}
}

class RunkitSubClass extends RunkitClass {
}

class RunkitSubSubClass extends RunkitSubClass {
    private $privateProperty = "overriden";
    function getPrivate1() {return $this->privateProperty;}
}

class RunkitSubSubSubClass extends RunkitSubSubClass {
}

ini_set('display_errors', 1);
ini_set('error_reporting', E_ALL);

$obj = new RunkitClass();
$objs = new RunkitSubClass();
$objss = new RunkitSubSubClass();
$objsss = new RunkitSubSubSubClass();

runkit_default_property_remove('RunkitClass', 'privateProperty', TRUE);
print_r($obj);
print_r($objs);
print_r($objss);
print_r($objsss);
echo $obj->getPrivate(), "\n";
echo $objs->getPrivate(), "\n";
echo $objss->getPrivate1(), "\n";
echo $objsss->getPrivate1(), "\n";
?>
--EXPECTF--
RunkitClass Object
(
)
RunkitSubClass Object
(
)
RunkitSubSubClass Object
(
    [privateProperty%sprivate] => overriden
)
RunkitSubSubSubClass Object
(
    [privateProperty%sprivate] => overriden
)

Notice: Undefined property: RunkitClass::$privateProperty in %s on line %d


Notice: Undefined property: RunkitSubClass::$privateProperty in %s on line %d

overriden
overriden

