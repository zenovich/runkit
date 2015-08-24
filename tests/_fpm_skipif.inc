<?php
if (substr(PHP_OS, 0, 3) == 'WIN') {
	die("skip: not for Windows");
}
$version = explode(".", PHP_VERSION);
if($version[0] < 5) {
	die("skip: PHP4 is not supported");
}
if($version[0] == 5 && $version[1] < 4) {
	die("skip: fpm always daemonizes in PHP below 5.4");
}
include dirname(__FILE__)."/_fpm_include.inc";
if (!get_fpm_path()) {
	die("skip: FPM not found");
}
?>
