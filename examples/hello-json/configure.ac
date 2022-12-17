AC_INIT([hello_json], [1.0], [user@example.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
])
AX_CXX_COMPILE_STDCXX(20)
PKG_CHECK_MODULES(CPPWINRT, [CppWinRT >= 2.0.221215.0])
AC_OUTPUT
