AC_INIT([gel], [0.4], [tigrux@gmail.com], [gel])
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

PKG_CHECK_MODULES(GI, gobject-introspection-1.0)
AC_SUBST(GI_CFLAGS)
AC_SUBST(GI_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

AC_CONFIG_FILES([
	Makefile
	libgel/Makefile
	vapi/Makefile
	examples/Makefile
	gel-0.1.pc
	docs/Makefile
	docs/reference/Makefile
	docs/reference/gel/Makefile
	])

AC_SEARCH_LIBS(g_info_type_to_string,
    girepository-1.0,
    AC_DEFINE(HAVE_G_INFO_TYPE_TO_STRING, 1, "Define to 1 if you have g_info_type_to_string")
)

AC_OUTPUT

