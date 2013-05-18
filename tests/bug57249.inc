<?php
class cBuggyClass {
  public function mBuggyMethod() {
    static $ls_a_iCache;
    echo 'mBuggyMethod();'.PHP_EOL;
  }
}
