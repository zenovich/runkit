--TEST--
runkit_default_property_remove() remove private properties with inheritance with objects overriding
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      $version = explode(".", PHP_VERSION);
      if($version[0] < 5) print "skip";
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

runkit_default_property_remove('RunkitSubClass', 'privateProperty', TRUE);
print_r(new RunkitClass());
print_r(new RunkitSubClass());
print_r(new RunkitSubSubClass());
print_r(new RunkitSubSubSubClass());
print_r($obj);
print_r($objs);
print_r($objss);
print_r($objsss);
echo $obj->getPrivate(), "\n";
echo $objs->getPrivate(), "\n";
echo $objss->getPrivate(), "\n";
echo $objsss->getPrivate(), "\n";
?>
--EXPECTF--
RunkitClass Object
(
    [privateProperty%sprivate] => original
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
RunkitClass Object
(
    [privateProperty%sprivate] => original
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
original

Notice: Undefined property: RunkitSubClass::$privateProperty in %s on line %d


Notice: Undefined property: RunkitSubSubClass::$privateProperty in %s on line %d


Notice: Undefined property: RunkitSubSubSubClass::$privateProperty in %s on line %d
