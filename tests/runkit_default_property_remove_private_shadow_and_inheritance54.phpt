--TEST--
runkit_default_property_remove() remove private properties with inheritance
--SKIPIF--
<?php if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
      $version = explode(".", PHP_VERSION);
      if($version[0] < 5) print "skip";
      if($version[0] == 5 && $version[1] < 4) print "skip";
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

runkit_default_property_remove('RunkitSubClass', 'privateProperty');
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
Notice: runkit_default_property_remove(): Making RunkitSubSubSubClass::privateProperty public to remove it from class without objects overriding in %s on line %d

Notice: runkit_default_property_remove(): Making RunkitSubSubClass::privateProperty public to remove it from class without objects overriding in %s on line %d

Notice: runkit_default_property_remove(): Making RunkitSubClass::privateProperty public to remove it from class without objects overriding in %s on line %d
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
    [privateProperty] => original
)
RunkitSubSubClass Object
(
    [privateProperty%sprivate] => overriden
    [privateProperty] => original
)
RunkitSubSubSubClass Object
(
    [privateProperty%sprivate] => overriden
    [privateProperty] => original
)
original
original
original
original

