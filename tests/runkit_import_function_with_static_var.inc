<?php
function f() {
	static $v = 1;
	$v++;
	echo $v, "\n";
}
