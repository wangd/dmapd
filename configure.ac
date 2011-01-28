AC_INIT(src/dmapd.c)

MAJOR_VERSION=0
MINOR_VERSION=0
MICRO_VERSION=37
AM_INIT_AUTOMAKE(dmapd, $MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION)

VER_INFO=`expr $MINOR_VERSION + $MAJOR_VERSION`:$MICRO_VERSION:$MINOR_VERSION

DMAPD_MAJORMINOR=$MAJOR_VERSION.$MINOR_VERSION
AC_SUBST(DMAPD_MAJORMINOR)
AC_SUBST(VER_INFO)

AM_CONFIG_HEADER(config.h)

AM_DISABLE_STATIC

AC_PROG_CC
AC_PROG_LIBTOOL

LOCKPATH='${localstatedir}/lock/subsys/dmapd'
AC_SUBST(LOCKPATH)

RUNDIR='${localstatedir}/run/dmapd'
AC_SUBST(RUNDIR)

MODULEDIR='${pkglibdir}/${VERSION}/modules'
AC_SUBST(MODULEDIR)

SYSCONFDIR='${sysconfdir}'
AC_SUBST(SYSCONFDIR)

MODULE_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(MODULE_LIBTOOL_FLAGS)

AC_ARG_ENABLE(daap, [  --disable-daap          do not build DAAP support], daap=$enableval, daap=yes)
if test x$daap == xyes; then
	AC_DEFINE(WITH_DAAP,1,[Defined when DAAP support is to be built])
	AM_CONDITIONAL(WITH_DAAP, true)
fi

AC_ARG_ENABLE(dpap, [  --disable-dpap          do not build DPAP support], dpap=$enableval, dpap=yes)
if test x$dpap == xyes; then
	AC_DEFINE(WITH_DPAP,1,[Defined when DPAP support is to be built])
	AM_CONDITIONAL(WITH_DPAP, true)
fi

dnl Check for pkgconfig
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)
dnl Give error and exit if we don't have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR(you need to have pkgconfig installed !)
fi

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
        AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
	[], [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$with_systemdsystemunitdir"])

DISTCHECK_CONFIGURE_FLAGS = \
        --with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

dnl Check for Glib2.0
PKG_CHECK_MODULES(GLIB, glib-2.0, HAVE_GLIB=yes,HAVE_GLIB=no)

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
PKG_CHECK_MODULES(DMAPSHARING, libdmapsharing-3.0,
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
PKG_CHECK_MODULES(VIPS, vips-7.24 >= 7.24.1,
  HAVE_VIPS=yes,
  HAVE_VIPS=no)

if test x"$HAVE_VIPS" = "xyes"; then
  AC_DEFINE(HAVE_VIPS, 1, [Define if vips support is enabled])
fi

AM_CONDITIONAL(USE_VIPS, test x"$HAVE_VIPS" = "xyes")

AC_SUBST(VIPS_CFLAGS)
AC_SUBST(VIPS_LIBS)

dnl Check for GStreamer, needed for DAAP metadata reading
PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10, HAVE_GSTREAMER=yes,
  HAVE_GSTREAMER=no)

if test x"$HAVE_GSTREAMER" = "xyes"; then
  AC_DEFINE(HAVE_GSTREAMER, 1, [Define if GStreamer support is enabled])
fi

AM_CONDITIONAL(USE_GSTREAMER, test x"$HAVE_GSTREAMER" = "xyes")

AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

dnl Check for inotify, used for media directory monitoring
AC_CHECK_HEADERS([sys/inotify.h])

dnl Check for Berkeley Database
# NOTE: AC_CHECK_LIB(db-4.8, ... passed even when headers not installed:
AC_CHECK_HEADER(db.h, HAVE_DB_H=yes, HAVE_DB_H=no)
if test x"$HAVE_DB_H" = "xyes"; then
  AC_CHECK_LIB(db-4.8, db_create, HAVE_LIBDB_4_8=yes, HAVE_LIBDB_4_8=no)
  AM_CONDITIONAL(USE_LIBDB_4_8, test x"$HAVE_LIBDB_4_8" = "xyes")
else
  AM_CONDITIONAL(USE_LIBDB_4_8, false)
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
