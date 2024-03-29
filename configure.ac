AC_INIT(src/dmapd.c)

MAJOR_VERSION=0
MINOR_VERSION=0
MICRO_VERSION=62
AM_INIT_AUTOMAKE(dmapd, $MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION)

VER_INFO=`expr $MINOR_VERSION + $MAJOR_VERSION`:$MICRO_VERSION:$MINOR_VERSION

DMAPD_MAJORMINOR=$MAJOR_VERSION.$MINOR_VERSION
AC_SUBST(DMAPD_MAJORMINOR)
AC_SUBST(VER_INFO)

AM_CONFIG_HEADER(config.h)

AM_DISABLE_STATIC

AC_PROG_CC
AC_PROG_LIBTOOL

DBDIR='${localstatedir}/db/dmapd'
AC_SUBST(DBDIR)

LOCKPATH='${localstatedir}/lock/subsys/dmapd'
AC_SUBST(LOCKPATH)

MODULEDIR='${pkglibdir}/${VERSION}/modules'
AC_SUBST(MODULEDIR)

RUNDIR='${localstatedir}/run/dmapd'
AC_SUBST(RUNDIR)

SYSCONFDIR='${sysconfdir}'
AC_SUBST(SYSCONFDIR)

MODULE_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(MODULE_LIBTOOL_FLAGS)

AC_ARG_ENABLE(daap, [  --disable-daap          do not build DAAP support], daap=$enableval, daap=yes)
if test x$daap = xyes; then
	AC_DEFINE(WITH_DAAP,1,[Defined when DAAP support is to be built])
	AM_CONDITIONAL(WITH_DAAP, true)
else
	AM_CONDITIONAL(WITH_DAAP, false)
fi

AC_ARG_ENABLE(dpap, [  --disable-dpap          do not build DPAP support], dpap=$enableval, dpap=yes)
if test x$dpap = xyes; then
	AC_DEFINE(WITH_DPAP,1,[Defined when DPAP support is to be built])
	AM_CONDITIONAL(WITH_DPAP, true)
else
	AM_CONDITIONAL(WITH_DPAP, false)
fi

AC_ARG_ENABLE(dacp, [  --disable-dacp          do not build DACP support], dacp=$enableval, dacp=yes)
if test x$dacp = xyes; then
	AC_DEFINE(WITH_DACP,1,[Defined when DACP support is to be built])
	AM_CONDITIONAL(WITH_DACP, true)
else
	AM_CONDITIONAL(WITH_DACP, false)
fi

AC_ARG_ENABLE(tests, [  --disable-tests         do not build tests], tests=$enableval, tests=yes)
if test x$tests = xyes; then
	AC_DEFINE(WITH_TESTS,1,[Defined when tests are to be built])
	AM_CONDITIONAL(WITH_TESTS, true)
else
	AM_CONDITIONAL(WITH_TESTS, false)
fi

dnl Check for pkgconfig
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)
dnl Give error and exit if we don't have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR(you need to have pkgconfig installed !)
fi

PKG_PROG_PKG_CONFIG

dnl Check for systemd
AC_ARG_WITH([systemdsystemunitdir],
AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files (auto=detect)]), [
        if test "$withval" = "auto"; then
                systemdsystemunitdir=`$PKG_CONFIG --variable=systemdsystemunitdir systemd`
        elif test "$withval" != "no"; then
                systemdsystemunitdir=$withval
        fi
], [])
if test "$systemdsystemunitdir" != ""; then
        AC_SUBST(systemdsystemunitdir)
        AC_DEFINE(HAVE_SYSTEMD,, Define if you want to use systemd socket activation)
fi
AM_CONDITIONAL(HAVE_SYSTEMD, test "$systemdsystemunitdir" != "")

dnl Check for Glib2.0
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.36, HAVE_GLIB=yes,HAVE_GLIB=no)

dnl Give error and exit if we don't have glib
if test "x$HAVE_GLIB" = "xno"; then
  AC_MSG_ERROR(you need glib-2.0 installed)
fi

dnl make GLIB_CFLAGS and GLIB_LIBS available
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AC_DEFINE(HAVE_GLIB,1,[Defined when glib-2.0 was found])

dnl Check for GThread2.0
PKG_CHECK_MODULES(GTHREAD, 
  gthread-2.0, 
  HAVE_GTHREAD=yes, HAVE_GTHREAD=no)

dnl Give error and exit if we don't have gthread
if test "x$HAVE_GTHREAD" = "xno"; then
  AC_MSG_ERROR(you need gthread-2.0 installed)
fi

dnl make GTHREAD_CFLAGS and GTHREAD_LIBS available
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

dnl Check for GObject2.0
PKG_CHECK_MODULES(GOBJECT, 
  gobject-2.0, 
  HAVE_GOBJECT=yes, HAVE_GOBJECT=no)

dnl Give error and exit if we don't have gobject
if test "x$HAVE_GOBJECT" = "xno"; then
  AC_MSG_ERROR(you need gobject-2.0 installed)
fi

dnl make GOBJECT_CFLAGS and GOBJECT_LIBS available
AC_SUBST(GOBJECT_CFLAGS)
AC_SUBST(GOBJECT_LIBS)

dnl Check for libexif
PKG_CHECK_MODULES(EXIF, libexif,
  HAVE_LIBEXIF=yes,
  HAVE_LIBEXIF=no)

if test x"$HAVE_LIBEXIF" = "xyes"; then
  AC_DEFINE(HAVE_LIBEXIF, 1, [Define if libexif support is enabled])
fi

AM_CONDITIONAL(USE_LIBEXIF, test x"$HAVE_LIBEXIF" = "xyes")

AC_SUBST(EXIF_CFLAGS)
AC_SUBST(EXIF_LIBS)

dnl Check for libdmapsharing
PKG_CHECK_MODULES(DMAPSHARING, libdmapsharing-3.0 >= 2.9.21,
  HAVE_LIBDMAPSHARING=yes,
  HAVE_LIBDMAPSHARING=no)

if test x"$HAVE_LIBDMAPSHARING" = "xyes"; then
  AC_DEFINE(HAVE_LIBDMAPSHARING, 1, [Define if libdmapsharing support is enabled])
fi

AM_CONDITIONAL(USE_LIBDMAPSHARING, test x"$HAVE_LIBDMAPSHARING" = "xyes")

dnl daap support
if test x"$HAVE_LIBDMAPSHARING" = "xno"; then
  AC_MSG_ERROR([libdmapsharing not found.  Install libdmapsharing])
fi

AC_SUBST(DMAPSHARING_CFLAGS)
AC_SUBST(DMAPSHARING_LIBS)

dnl Check for GraphicsMagick, used for DPAP thumbnailing
PKG_CHECK_MODULES(MAGICK, GraphicsMagickWand,
  HAVE_MAGICK=yes,
  HAVE_MAGICK=no)

if test x"$HAVE_MAGICK" = "xyes"; then
  AC_DEFINE(HAVE_MAGICK, 1, [Define if GraphicsMagickWand support is enabled])
fi

AM_CONDITIONAL(USE_MAGICK, test x"$HAVE_MAGICK" = "xyes")

AC_SUBST(MAGICK_CFLAGS)
AC_SUBST(MAGICK_LIBS)

dnl Check for libvips, used for DPAP thumbnailing
PKG_CHECK_MODULES(VIPS, vips,
  HAVE_VIPS=yes,
  HAVE_VIPS=no)

AM_CONDITIONAL(USE_VIPS, test x"$HAVE_VIPS" = "xyes")

AC_SUBST(VIPS_CFLAGS)
AC_SUBST(VIPS_LIBS)

dnl Check for GStreamer, needed for DAAP metadata reading
PKG_CHECK_MODULES(GSTREAMER, gstreamer-1.0, HAVE_GSTREAMER=yes,
  HAVE_GSTREAMER=no)

if test x"$HAVE_GSTREAMER" = "xyes"; then
  AC_DEFINE(HAVE_GSTREAMER, 1, [Define if GStreamer support is enabled])
fi

AM_CONDITIONAL(USE_GSTREAMER, test x"$HAVE_GSTREAMER" = "xyes")

AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

dnl Check for inotify, used for media directory monitoring
AC_CHECK_HEADERS([sys/inotify.h])

dnl Check for Berkeley Database 4.8
# NOTE: AC_CHECK_LIB(db-4.8, ... passed even when headers not installed:
AC_CHECK_HEADER(db.h, HAVE_DB_H=yes, HAVE_DB_H=no)
if test x"$HAVE_DB_H" = "xyes"; then
  AC_CHECK_LIB(db-4.8, db_create, HAVE_LIBDB_4_8=yes, HAVE_LIBDB_4_8=no)
  AM_CONDITIONAL(USE_LIBDB_4_8, test x"$HAVE_LIBDB_4_8" = "xyes")
else
  AM_CONDITIONAL(USE_LIBDB_4_8, false)
fi

dnl Check for Berkeley Database 4.7
# NOTE: AC_CHECK_LIB(db-4.7, ... passed even when headers not installed:
AC_CHECK_HEADER(db.h, HAVE_DB_H=yes, HAVE_DB_H=no)
if test x"$HAVE_DB_H" = "xyes"; then
  AC_CHECK_LIB(db-4.7, db_create, HAVE_LIBDB_4_7=yes, HAVE_LIBDB_4_7=no)
  AM_CONDITIONAL(USE_LIBDB_4_7, test x"$HAVE_LIBDB_4_7" = "xyes")
else
  AM_CONDITIONAL(USE_LIBDB_4_7, false)
fi

dnl Set overall Berkeley Database conditional
if test x"$HAVE_LIBDB_4_7" = "xyes" -o x"$HAVE_LIBDB_4_8" = "xyes"; then
  AM_CONDITIONAL(USE_LIBDB, true)
else
  AM_CONDITIONAL(USE_LIBDB, false)
fi

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4],have_check=yes,have_check=no)
AM_CONDITIONAL(HAVE_CHECK, test x"$have_check" = "xyes")

AM_CONDITIONAL(FLYN, test "$FLYN")

AC_OUTPUT([
dmapd.pc
Makefile
distro/Makefile
dry/Makefile
src/Makefile
])
