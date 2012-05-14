AC_INIT([gel], [0.5], [tigrux@gmail.com], [gel])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

AC_PROG_CC_C99
AC_PROG_LIBTOOL
AC_DISABLE_STATIC

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

PKG_CHECK_MODULES(GOBJECT, gobject-2.0)
AC_SUBST(GOBJECT_CFLAGS)
AC_SUBST(GOBJECT_LIBS)

PKG_CHECK_MODULES(GI, gobject-introspection-1.0,
    HAVE_GOBJECT_INTROSPECTION=1
    AC_DEFINE(HAVE_GOBJECT_INTROSPECTION, 1,
              Define to 1 if gobject-introspection is installed)
    PACKAGE_REQUIRES="gobject-2.0 gobject-introspection-1.0",
    HAVE_GOBJECT_INTROSPECTION=0
    PACKAGE_REQUIRES="gobject-2.0")

AC_SUBST(PACKAGE_REQUIRES)
AC_SUBST(GI_CFLAGS)
AC_SUBST(GI_LIBS)

GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

AC_CONFIG_FILES([
	Makefile
	bin/Makefile
	libgel/Makefile
	examples/Makefile
	gel-1.0.pc
	docs/Makefile
	docs/reference/Makefile
	docs/reference/gel/Makefile
	])

AC_SEARCH_LIBS(g_info_type_to_string,
    girepository-1.0,
    AC_DEFINE(HAVE_G_INFO_TYPE_TO_STRING, 1,
              Define to 1 if g_info_type_to_string is provided)
)

AC_SEARCH_LIBS(g_constant_info_free_value,
    girepository-1.0,
    AC_DEFINE(HAVE_G_CONSTANT_INFO_FREE_VALUE, 1,
              Define to 1 if g_constant_info_free_value is provided)
)

AM_CONDITIONAL(HAVE_GOBJECT_INTROSPECTION, test $HAVE_GOBJECT_INTROSPECTION = 1)

AC_OUTPUT

if test $HAVE_GOBJECT_INTROSPECTION = 0; then
    echo
    echo "gobject-introspection-1.0 is not installed"
    echo "You can continue but it's strongly recommended that you install it"
    echo
fi

