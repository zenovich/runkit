Runkit extension for PHP
========================

For all those things you.... probably shouldn't have been doing anyway....

Runkit has three groups of features outlined below.  To install runkit as a module for PHP, you use PECL:

	sudo pecl install https://github.com/zenovich/runkit/archive/1.0.3.tar.gz


Custom Superglobals
-------------------

A new .ini entry `runkit.superglobal` is defined which may be specified as a simple variable, or list of simple variables to be registered as superglobals.  runkit.superglobal is defined as PHP_INI_SYSTEM and must be set in the system-wide php.ini.

### Example:

php.ini:

	runkit.superglobal=foo,bar

test.php:

	function testme() {
	  echo "Foo is $foo\n";
	  echo "Bar is $bar\n";
	  echo "Baz is $baz\n";
	}
	$foo = 1;
	$bar = 2;
	$baz = 3;

	testme();

Outputs:

	Foo is 1
	Bar is 2
	Baz is


Compatability: PHP 4.2 or greater


User Defined Function and Class Manipulation
--------------------------------------------

User defined functions and userdefined methods may now be renamed, delete, and redefined using the API described at http://www.php.net/runkit

Examples for these functions may also be found in the tests folder.

Compatability: PHP4 and PHP5


Sandboxing
----------

With the introduction of TSRM based subinterpreter support a running PHP script may now generate a new thread and interactively switch contexts back and forth between it.  THIS FEATURE DOES NOT PROVIDE FULL SCRIPT THREADING.  This feature only allows you to run processes in a subinterpreter optionally with additional security.

First, create an instance of the Runkit_Sandbox object:

	$php = new Runkit_Sandbox();

To read and write variables in this subinterpreter, just access the properties of the object:

	$php->foo = 'bar';
	$php->baz = 'boom';

Individual functions may also be called (executed within the newly created scope):

	$php->session_start();

Or you can execute a block of arbitrary code:

	$php->eval('echo "The value of foo is $foo\n";');

In this example, $foo will be interpolated as 'bar' since that's what you set it to earlier.

Certain INI Options which are ordinarily only modifiable in the system php.ini may be passed during instantiation and changed for your subinterpreter as well, these options are passed as an associative array to the `Runkit_Sandbox` constructor and include the following:

* `safe_mode` - `safe_mode` may only be turned on for a `Runkit_Sandbox` interpreter using this option.  It cannot be turned off, doing so would circumvent the setting specified by your system administrator in the system php.ini.

* `open_basedir` - Like `safe_mode`, you can only use this setting to make things more restrictive.

* `allow_url_fopen` - In keeping with `safe_mode`, this can only be turned off (more restrictive than global environment).

* `disable_functions` - Any function names specified in this comma-delimited list will be disabled *in addition to* already disabled functions.

* `disable_classes` - Like `disable_functions`, this list is in addition to already disabled classes.

Sandboxing is *only available* in PHP 5.1 (release version, or snapshot dated after April 28th, 2005) when thread safety has been enabled.  To enable thread safety, just make sure that `--enable-maintainer-zts` is specified on your `./configure line`.  This doesn't necessarily mean that your SAPI will use PHP in a threaded manner, just that PHP is prepared to behave that way.  If you're building for Apache2-Worker then you're already built for thread safety.

If you wish/need to use PHP 5.0.x, or a cvs snapshot of 5.1 which predates April 28th, you can apply the `tsrm_5.0.diff` patch included in this package:

	cd /path/to/php-5.0.x/
	cat /path/to/runkit/tsrm_5.0.diff | patch -p0

Then just rebuild using the `--enable-maintainer-zts` option specified above.

`runkit_lint()` and `runkit_lint_file()` also exist as a simpler approach to verifying the syntactic legality of passed code within an isolated environment.


Building the Runkit Module for Windows
--------------------------------------

First, place source code of runkit into a temporary directory, for example `C:\runkit-source`.  Open your Windows SDK command prompt or Visual Studio Command prompt.  Then change into the runkit's source code directory:

	C:
	cd C:\runkit-source

After that, run phpize from your PHP SDK. This may be something like

	C:\php\SDK\phpize.bat

Then configure your runkit module by executing "configure". You can view the full list of options by the command

	configure --help

but in most cases, you probably will choose a simple command

	configure --enable-runkit

After all run

	nmake

Now you should have the `php_runkit.dll` file.
