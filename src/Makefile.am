INCLUDES = \
        -I$(top_srcdir) \
	$(DMAPSHARING_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(EXIF_CFLAGS) \
	$(AVAHI_CFLAGS) \
	$(MAGICK_CFLAGS) \
	$(GSTREAMER_CFLAGS) \
	$(SOUP_CFLAGS)

AM_CFLAGS = \
	-DDEFAULT_DBDIR=\"$(DBDIR)\" \
	-DDEFAULT_LOCKPATH=\"$(LOCKPATH)\" \
	-DDEFAULT_MODULEDIR=\"$(MODULEDIR)\" \
	-DDEFAULT_RUNDIR=\"$(RUNDIR)\" \
	-DDEFAULT_SYSCONFDIR=\"$(SYSCONFDIR)\"

sbin_PROGRAMS = dmapd

if USE_GSTREAMER
bin_PROGRAMS = \
	dmapd-transcode \
	dmapd-hashgen
endif

noinst_PROGRAMS = dmapd-stress-test

if HAVE_CHECK
noinst_PROGRAMS += dmapd-unit-test
endif

dmapd_SOURCES = \
	dmapd.c

dmapd_LDADD = libdmapd.la

dmapd_transcode_SOURCES = \
	dmapd-transcode.c

dmapd_transcode_LDADD = libdmapd.la

dmapd_hashgen_SOURCES = \
	dmapd-hashgen.c

dmapd_hashgen_LDADD = libdmapd.la

dmapd_stress_test_SOURCES = \
	dmapd-stress-test.c

dmapd_stress_test_LDADD = libdmapd.la

if HAVE_CHECK
dmapd_unit_test_SOURCES = \
	dmapd-unit-test.c \
	dmapd-test-daap-record.c \
	dmapd-test-parse-plugin-option.c

dmapd_unit_test_LDADD = libdmapd.la @CHECK_LIBS@
dmapd_unit_test_INCLUDES = @CHECK_CFLAGS@
endif

lib_LTLIBRARIES = libdmapd.la

libdmapd_la_SOURCES = \
	util.c \
	av-meta-reader.c \
	av-render.c \
	db-builder.c \
	dmapd-dmap-container-db.c \
	dmapd-dmap-container-record.c \
	dmapd-dmap-db.c \
	dmapd-dmap-db-ghashtable.c \
	dmapd-daap-record.c \
	dmapd-daap-record-factory.c \
	dmapd-dpap-record.c \
	dmapd-dpap-record-factory.c \
	dmapd-module.c \
	photo-meta-reader.c

libdmapd_la_LIBADD = \
	$(DMAPSHARING_LIBS) \
	$(GSTREAMER_LIBS)

libdmapd_la_LDFLAGS = -version-info @VER_INFO@

plugindir = $(MODULEDIR)
plugin_LTLIBRARIES = \
	libdb-builder-gdir.la \
	libdmapd-dmap-db-disk.la

if USE_LIBDB
plugin_LTLIBRARIES += \
	libdmapd-dmap-db-bdb.la
endif

if USE_GSTREAMER
libdmapd_la_SOURCES += util-gst.c

plugin_LTLIBRARIES += \
	libav-meta-reader-gst.la \
	libav-render-gst.la

libav_meta_reader_gst_la_SOURCES = \
	av-meta-reader-gst.c

libav_meta_reader_gst_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libav_meta_reader_gst_la_LIBADD = \
	$(GSTREAMER_LIBS)

libav_render_gst_la_SOURCES = \
	av-render-gst.c

libav_render_gst_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libav_render_gst_la_LIBADD = \
	$(GSTREAMER_LIBS)
endif

if USE_MAGICK
plugin_LTLIBRARIES += \
	libphoto-meta-reader-graphicsmagick.la

libphoto_meta_reader_graphicsmagick_la_SOURCES = \
	photo-meta-reader-graphicsmagick.c

libphoto_meta_reader_graphicsmagick_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libphoto_meta_reader_graphicsmagick_la_LIBADD = \
	$(EXIF_LIBS) \
	$(MAGICK_LIBS)
endif

if USE_VIPS
plugin_LTLIBRARIES += \
	libphoto-meta-reader-vips.la

libphoto_meta_reader_vips_la_SOURCES = \
	photo-meta-reader-vips.c

libphoto_meta_reader_vips_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libphoto_meta_reader_vips_la_LIBADD = \
	$(VIPS_LIBS)
endif

libdb_builder_gdir_la_SOURCES = \
	db-builder-gdir.c

libdb_builder_gdir_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

libdmapd_dmap_db_disk_la_SOURCES = \
	dmapd-dmap-db-disk.c

libdmapd_dmap_db_disk_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

if USE_LIBDB
libdmapd_dmap_db_bdb_la_SOURCES = \
        dmapd-dmap-db-bdb.c

libdmapd_dmap_db_bdb_la_LDFLAGS = $(MODULE_LIBTOOL_FLAGS)

if USE_LIBDB_4_8
libdmapd_dmap_db_bdb_la_LIBADD = \
	-ldb-4.8
else
libdmapd_dmap_db_bdb_la_LIBADD = \
	-ldb-4.7
endif
endif

dmapdincludedir = \
	$(includedir)/dmapd-@DMAPD_MAJORMINOR@/dmapd

dmapdinclude_HEADERS = \
	dmapd.h \
	dmapd-dmap-db.h \
	av-meta-reader.h \
	av-render.h \
	photo-meta-reader.h \
	db-builder.h \
	dmapd-module.h

noinst_HEADERS = \
	util.h \
	util-gst.h \
	dmapd-daap-record.h \
	dmapd-dmap-container-db.h \
	dmapd-dmap-container-record.h \
	dmapd-dmap-db-bdb.h \
	dmapd-dmap-db-disk.h \
	dmapd-dmap-db-ghashtable.h \
	av-meta-reader-gst.h \
	av-render-gst.h \
	photo-meta-reader-graphicsmagick.h \
	photo-meta-reader-vips.h \
	db-builder-gdir.h \
	dmapd-dpap-record.h \
	dmapd-dpap-record-factory.h \
	dmapd-daap-record-factory.h \
	dmapd-test-daap-record.h \
	dmapd-test-parse-plugin-option.h
