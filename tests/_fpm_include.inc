<?php
function get_fpm_path() /* {{{ */
{
	$php_path = getenv("TEST_PHP_EXECUTABLE");
	for ($i = 0; $i < 2; $i++) {
		$slash_pos = strrpos($php_path, "/");
		if ($slash_pos) {
			$php_path = substr($php_path, 0, $slash_pos);
		} else {
			return false;
		}
	}
	if ($php_path && is_dir($php_path)) {
		if (file_exists($php_path."/fpm/php-fpm") && is_executable($php_path."/fpm/php-fpm")) {
			/* gotcha */
			return $php_path."/fpm/php-fpm";
		}
		if (file_exists($php_path."/sbin/php-fpm") && is_executable($php_path."/sbin/php-fpm")) {
			/* gotcha */
			return $php_path."/sbin/php-fpm";
		}
	}
	return false;
}
/* }}} */

function _fpm_shutdown_function($fpm, $cfg) {
	@unlink($cfg);
	if (is_resource($fpm)) {
		@proc_terminate($fpm);
		while (($status = proc_get_status($fpm)) && $status['running']) {
			usleep(10000);
		}
	}
}

function run_fpm($config, &$out = false, $extra_args = '') /* {{{ */
{
    $cfg = dirname(__FILE__).'/test-fpm-config.tmp';
    file_put_contents($cfg, $config);
    $desc = array();
    if ($out !== false) {
        $desc[1] = array('pipe', 'w');
    }
    /* Since it's not possible to spawn a process under linux without using a
     * shell in php (why?!?) we need a little shell trickery, so that we can
     * actually kill php-fpm */
    //echo file_get_contents('../tmp-php.ini');
    $fpm = proc_open('killit () { kill $child; }; trap killit TERM; '.get_fpm_path().' -O -y '.$cfg." ".$extra_args.' 2>&1 & child=$!; wait', $desc, $pipes);
    register_shutdown_function('_fpm_shutdown_function', $fpm, $cfg);
    if ($out !== false) {
        $out = $pipes[1];
    }
    return $fpm;
}
/* }}} */
function run_fpm_till($needle, $config, $max = 10) /* {{{ */
{
    $i = 0;
    $fpm = run_fpm($config, $tail);
    if (is_resource($fpm)) {
        while($i < $max) {
            $i++;
            $line = fgets($tail);
            if(preg_match($needle, $line) === 1) {
                break;
            }
        }
        if ($i >= $max) {
            $line = false;
        }
        proc_terminate($fpm);
        stream_get_contents($tail);
        fclose($tail);
        proc_close($fpm);
    }
    return $line;
}
/* }}} */
function fpm_display_log($tail, $n=1, $ignore='systemd') {
	while ($n) {
		$a = fgets($tail);
		if (empty($ignore) || !strpos($a, $ignore)) {
			echo $a;
			$n--;
		}
	}
}
function run_request($host, $port, $uri='/ping', $query='') {
	require_once '_fcgi.inc';
	$client = new Adoy_FastCGI_Client($host, $port);
	$params = array(
		'GATEWAY_INTERFACE' => 'FastCGI/1.0',
		'REQUEST_METHOD'    => 'GET',
		'SCRIPT_FILENAME'   => $uri,
		'SCRIPT_NAME'       => $uri,
		'QUERY_STRING'      => $query,
		'REQUEST_URI'       => $uri . ($query ? '?'.$query : ""),
		'DOCUMENT_URI'      => $uri,
		'SERVER_SOFTWARE'   => 'php/fcgiclient',
		'REMOTE_ADDR'       => '127.0.0.1',
		'REMOTE_PORT'       => '9985',
		'SERVER_ADDR'       => '127.0.0.1',
		'SERVER_PORT'       => '80',
		'SERVER_NAME'       => php_uname('n'),
		'SERVER_PROTOCOL'   => 'HTTP/1.1',
		'CONTENT_TYPE'      => '',
		'CONTENT_LENGTH'    => 0
	);
	return $client->request($params, false)."\n";
}

function fpm_test($code, $extra_args) {
	$logfile = dirname(__FILE__).'/php-fpm.log.tmp';
	$srcfile = dirname(__FILE__).'/php-fpm.tmp.php';
	$port = 9000+PHP_INT_SIZE;
	$cfg = <<<EOT
[global]
error_log = $logfile
daemonize = no
[unconfined]
listen = 127.0.0.1:$port
pm = dynamic
pm.max_children = 1
pm.start_servers = 1
pm.min_spare_servers = 1
pm.max_spare_servers = 1
php_flag[display_errors] = on
EOT;
	if (!is_array($code)) {
		$code = array($code);
	}
	$fpm = run_fpm($cfg, $tail, $extra_args);
	if (is_resource($fpm)) {
		fpm_display_log($tail, 2);
		foreach ($code as $code_str) {
			file_put_contents($srcfile, $code_str);
			try {
				$req = run_request('127.0.0.1', $port, $srcfile);
				echo strstr($req, "Test Start");
				echo "Request ok\n";
			} catch (Exception $e) {
				echo "Request error\n";
			}
		}
		proc_terminate($fpm);
		echo stream_get_contents($tail);
		fclose($tail);
		proc_close($fpm);
	}
}

function fpm_clean() {
	$logfile = dirname(__FILE__).'/php-fpm.log.tmp';
	$srcfile = dirname(__FILE__).'/php-fpm.tmp.php';
	@unlink($logfile);
	@unlink($srcfile);
}
