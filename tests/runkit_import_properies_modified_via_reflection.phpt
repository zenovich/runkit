--TEST--
runkit_import() Importing and overriding properties which were modified via reflection
--SKIPIF--
<?php
    if(!extension_loaded("runkit") || !RUNKIT_FEATURE_MANIPULATION) print "skip";
    if(version_compare(PHP_VERSION, '5.3.0', '<')) print "skip";
?>
--FILE--
<?php
require_once('runkit_import_properies_modified_via_reflection.inc');

$oTestReload = new TestReload('TestClass');
unset($oTestReload);

class TestReload{
	private $refClass;

	public function TestReload($sClass) {
		$this->refClass = new ReflectionClass($sClass);

		// Verify default property values
		$this->GetProperties();

		// Change property values
		$this->SetProperties('test');

		// Verify property values were changed
		$this->GetProperties();

		// Reload class
		$oReload = new Reload('runkit_import_properies_modified_via_reflection.inc');
		unset($oReload);

		// Verify the property values were reset to default
		$this->GetProperties();

		unset($this->refClass);
	}

	private function GetProperties() {
		$aProps = $this->refClass->getStaticProperties();
		var_dump($aProps);
	}

	private function SetProperties($sValue) {
		$aProps = $this->refClass->getStaticProperties();
		foreach($aProps as $sKey => $oProp) {
			$refProp = $this->refClass->getProperty($sKey);
			$refProp->setAccessible(true);
			$refProp->setValue($sValue);
			unset($refProp);
		}
	}
}

class Reload {
	public function Reload($sClassPath) {
		runkit_import($sClassPath, (RUNKIT_IMPORT_OVERRIDE|RUNKIT_IMPORT_CLASS_STATIC_PROPS));
	}
}
?>
--EXPECT--
array(1) {
  ["property"]=>
  NULL
}
array(1) {
  ["property"]=>
  string(4) "test"
}
array(1) {
  ["property"]=>
  NULL
}

