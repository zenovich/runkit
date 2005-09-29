dnl $Id$
dnl config.m4 for extension runkit

PHP_ARG_ENABLE(runkit, whether to enable runkit support,
[  --enable-runkit           Enable runkit support])

if test "$PHP_RUNKIT" != "no"; then
  if test "$PHP_RUNKIT" = "classkit"; then
    AC_DEFINE(PHP_RUNKIT_CLASSKIT_COMPAT, 1, [Whether to export classkit compatable function aliases])
  fi
  PHP_NEW_EXTENSION(runkit, runkit.c runkit_functions.c runkit_methods.c \
runkit_constants.c runkit_import.c runkit_classes.c \
runkit_sandbox.c runkit_sandbox_parent.c, $ext_shared)
fi
