#!/bin/sh

aclocal || exit 1
autoconf || exit 1
autoheader || exit 1
libtoolize --force || glibtoolize --force || exit 1
automake -a || exit 1
./configure $* || exit 1

git log > ChangeLog

cat <<EOF >> ChangeLog

======================== Convert to Git-based ChangeLog ========================

21 November 2010 W. Michael Petullo <mike@flyn.org>

	* Make thumbnail optional within DPAPRecord. I am having trouble
	with the memory use of libjpeg when decompressing multiscan
	JPEG's. I have modified my copy of VIPS to skip multiscan
	files. In this case, the resulting DPAPRecord does not have
	a thumbnail, but memory usage is managable on small systems
	(e.g., 32MB memory). I am presently working with the VIPS
	developers to resolve this.

19 November 2010 W. Michael Petullo <mike@flyn.org>

	* Reduce maximum size of VIPS in memory image manipulation from
	1MB to 512KB.

	* Don't store location in DmapdDMAPDbDisk's db (location is
	still in the actual DPAP records, of course), reducing memory
	usage by 200KB when sharing a 4,000-photograph database with a
	reasonable database directory path.

	* Fix crash on OpenWrt by commenting out VIPS EXIF comment code.

18 November 2010 W. Michael Petullo <mike@flyn.org>

	* Fix minor memory issues identified by valgrind.

15 November 2010 W. Michael Petullo <mike@flyn.org>

	* Fix compile warnings.

	* Port test code to new modular database backends.

10 November 2010 W. Michael Petullo <mike@flyn.org>

	* Ensure modules are never loaded twice.

06 November 2010 W. Michael Petullo <mike@flyn.org>

	* Work on BDB module: Apply iPhoto G_MAXUINT vs. G_MAXINT (see
	previous for GHashTable DB); fix double-free of DPAP records'
	stringletons.

04 November 2010 W. Michael Petullo <mike@flyn.org>

	* Move from libdb-4.7 to libdb-48.

	* Begin modularizing backend DB code.

17 October 2010 W. Michael Petullo <mike@flyn.org>

	* Delete temporary VIPS thumbnail files when done with them.

16 October 2010 W. Michael Petullo <mike@flyn.org>

	* Fix missing NULL terminators for g_object_set(..."mediakind"...)

15 October 2010 W. Michael Petullo <mike@flyn.org>

	* Allow configuring of group (was hard-coded to "daemon").

30 September 2010 W. Michael Petullo <mike@flyn.org>

	* Implement suggestions from Maxim Bourmistrov: clean up default
	module code, add example to dmapd.conf, replace g_memdup and
	disallow both DAAP and DPAP data written to same cache file.

26 September 2010 W. Michael Petullo <mike@flyn.org>

	* Use max int, not max unsigned int for first DMAPDb ID. iPhoto
	6 does not seem to be able to handle max unsigned int (was seeing
	ID=-x in requests).

20 September 2010 W. Michael Petullo <mike@flyn.org>

	* Warn, don't error out if a module is missing.

12 September 2010 W. Michael Petullo <mike@flyn.org>

	* Remove check that forbid serving pictures and music while
	using -f option.

10 September 2010 W. Michael Petullo <mike@flyn.org>

	* Fix failed assertion having to do with empty playlists.

09 September 2010 W. Michael Petullo <mike@flyn.org>

	* Do not fork a process for each DMAP sub-protocol.

09 September 2010 W. Michael Petullo <mike@flyn.org>

	* Check for NULL before unref'ing to fix segfault on OpenWrt.

07 September 2010 W. Michael Petullo <mike@flyn.org>

	* Use DMAPMediaKind enum.

	* Change mistaken gboolean for mediakind to gint.

06 September 2010 W. Michael Petullo <mike@flyn.org>

	* Support DAAPRecord's sort-album and sort-artist properties.

	* Fix serving to iTunes 10: support com.apple.itunes.mediakind.

23 August 2010 W. Michael Petullo <mike@flyn.org>

	* Fix a memory leak in VIPS usage found using valgrind.

22 August 2010 W. Michael Petullo <mike@flyn.org>

	* Fix crash due to trying to load non-image using image reader.

	* Fix thumbnailing code in VIPS plugin (aspect ratio MUST be valid).

19 August 2010 W. Michael Petullo <mike@flyn.org>

	* Fix identifying duration of media files (forgot to set
	requested format to GST_FORMAT_TIME; this variable is used
	for both input and output for/from gst_element_query_duration ()).

18 August 2010 W. Michael Petullo <mike@flyn.org>

	* Adopt songalbum over daap.songalbum for DAAP record properties
	in accordance with new libdmapsharing.

06 August 2010 W. Michael Petullo <mike@flyn.org>

	* Add systemd .service file.

22 June 2010 W. Michael Petullo <mike@flyn.org>

	* Make Berkeley DB backend optional. Will eventually turn the
	DB backends into modules that may be loaded at runtime.

03 June 2010 W. Michael Petullo <mike@flyn.org>

	* Do not strdup before returning a string property of a
	DmapdD[AP]ARecord.

02 June 2010 W. Michael Petullo <mike@flyn.org>

	* Implement stringletons -- string singletons -- to further
	reduce memory usage. One use case reduced heap from 3,346,432
	to 2,826,249 bytes.

29 May 2010 W. Michael Petullo <mike@flyn.org>
	
	* Implement persistent databases.

28 May 2010 W. Michael Petullo <mike@flyn.org>

	* Fix compiler warnings.

27 May 2010 W. Michael Petullo <mike@flyn.org>

	* Free avahi client when finalizing DMAPShare.

	* Fix prototype for signal handlers.

	* Fix several items that resulted in compiler warnings.

26 May 2010 W. Michael Petullo <mike@flyn.org>

	* Add finalize code to DmapdDMAPContainerRecord.

25 May 2010 W. Michael Petullo <mike@flyn.org>

	* Bad assertion fixes.

	* Fix some memory leaks identified by valgrind.

23 May 2010 W. Michael Petullo <mike@flyn.org>

	* Fix dmapd_dmap_container_db_foreach, I don't know why I was
	using GFunc instead of GHFunc.

22 May 2010 W. Michael Petullo <mike@flyn.org>

	* Berkeley Database work.

	* Add db_dir configuration option.

20 May 2010 W. Michael Petullo <mike@flyn.org>

	* Add finalize code for:
		DmapdDMAPContainerDb
		DmapdDMAPDbBDB
		DmapdDMAPDbGHashTable

	* Free GObjectContext * in main.

	* Fix debug statement of has_video.

	* Use gst_object_unref instead of g_object_unref when appropriate.

18 May 2010 W. Michael Petullo <mike@flyn.org>

	* Do not try to set options if /etc/dmapd.conf does not exist.

	* Work on VIPS backend. This should allow thumbnail generation
	on low-memory systems.

17 May 2010 W. Michael Petullo <mike@flyn.org>

	* Continued to work memory usage issues. Found a few small and
	one larger memory leak in av-meta-reader-gst.c. The large leak
	is troublesome because its fix is to free a const gchar *. See
	FIXME comments is source file.

16 May 2010 W. Michael Petullo <mike@flyn.org>

	* Added Berkeley Database backend while trying to get to the
	bottom of memory usage. There seems to be a memory leak caused
	by av_meta_reader_gst_read, so more work is needed.

17 February 2010 W. Michael Petullo <mike@flyn.org>

	* Change dmapd.conf to set user by default.

05 February 2010 W. Michael Petullo <mike@flyn.org>

	* Parent kills children when SIGTERM is received.

	* Demote warning about unused metadata to a debug message.

31 January 2010 W. Michael Petullo <mike@flyn.org>

	* Add OpenWRT init script.

	* Various file cleanups.

	* Fix crash when a user provides a bad module path (e.g., to
	DMAPD_PHOTO_MODULE).

28 January 2010 W. Michael Petullo <mike@flyn.org>

	* Use lockfile=/var/lock/subsys/dmapd in init script to appease
	rpmlint (used /var/lock/subsys/$prog before).

	* Create default configuration file.

16 January 2010 W. Michael Petullo <mike@flyn.org>

	* Add configuration file support.

	* Provide more descriptive error if a media directory can't be read.

17 December 2009 W. Michael Petullo <mike@flyn.org>

	* Add vips plugin.

12 December 2009 W. Michael Petullo <mike@flyn.org>

	* Don't segfault if a GStreamer plugin is not available while
	reading AV metadata.

05 December 2009 W. Michael Petullo <mike@flyn.org>

	* Properly use new libdmapsharing log domain.

24 November 2009 W. Michael Petullo <mike@flyn.org>

	* Allow the user to set meta reader modules to "null" if he does
	not want to load any module.

22 November 2009 W. Michael Petullo <mike@flyn.org>

	* Move default data dir from /var/db/Storage to /var/db/dmapd.

	* Add option to drop root privileges.

10 November 2009 W. Michael Petullo <mike@flyn.org>

	* Set library version properly.

28 August 2009 W. Michael Petullo <mike@flyn.org>

	* Remove set_transcode function.

17 August 2009 W. Michael Petullo <mike@flyn.org>

	* Ensure dmapd-d[ap]ap-record.h is put in tarball.

13 August 2009 W. Michael Petullo <mike@flyn.org>

	* Proper reference counting of DMAPRecords in DMAPDbs.

10 August 2009 W. Michael Petullo <mike@flyn.org>

	* Reduce the number of installed header files.

08 August 2009 W. Michael Petullo <mike@flyn.org>

	* Fix issues related to building a module external to the dmapd
	source tree.

07 August 2009 W. Michael Petullo <mike@flyn.org>

	* Fix crash in new GTypeModule photo code.

06 August 2009 W. Michael Petullo <mike@flyn.org>

	* Complete new GTypeModule module work.

04 August 2009 W. Michael Petullo <mike@flyn.org>

	* Port DbBuilder to new API.

02 August 2009 W. Michael Petullo <mike@flyn.org>

	* Just include <libdmapdsharing/dmap.h>

	* Began to refactor plugin code.

31 July 2009 W. Michael Petullo <mike@flyn.org>

	* Use g_module_build_path instead of g_strconcat.

	* New module GObjects.

24 July 2009 W. Michael Petullo <mike@flyn.org>

	* Don't create versioned module libraries.

22 July 2009 W. Michael Petullo <mike@flyn.org>

	* Fix warning: DmapdD[AP]APRecord now implements DMAPRecord
	and D[AP]APRecord.

	* Fixed various runtime warnings.

21 July 2009 W. Michael Petullo <mike@flyn.org>

	* Fix handling of DAAP filesize.

20 July 2009 W. Michael Petullo <mike@flyn.org>

	* Handle errors when reading metadata; don't add to database.

19 July 2009 W. Michael Petullo <mike@flyn.org>

	* Make path to lock- and pidfile configurable at runtime.

13 July 2009 W. Michael Petullo <mike@flyn.org>

	* Move libmeta-gst.so to libav-meta-gst.so.

	* Create libphoto-meta-graphicsmagick.so.

10 July 2009 W. Michael Petullo <mike@flyn.org>

	* Create libdb-gdir.so and libmeta-gst.so modules.

05 July 2009 W. Michael Petullo <mike@flyn.org>

	* Move transcoding to libdmapsharing.

02 July 2009 W. Michael Petullo <mike@flyn.org>

	* Get rid of get methods in dmapd-daap-record.c and replace with
	GObject properties.

28 June 2009 W. Michael Petullo <mike@flyn.org>

	* Continue work to delay determining transcode format.

17 June 2009 W. Michael Petullo <mike@flyn.org>

	* Delay determining transcode format.

11 June 2009 W. Michael Petullo <mike@flyn.org>

	* Began to set the conditions for pulling GStreamer code into
	a dynamic module.

10 June 2009 W. Michael Petullo <mike@flyn.org>

	* Started to make DAAP or DPAP support optional.

01 May 2009 W. Michael Petullo <mike@flyn.org>

	* Increased the time waited for the decoded buffer to have room
	before dropping transcoded data in an effort to fix
	skipping. Previously audio would skip when decoding outpaced
	the client's ability to read from the network (especially
	prevalent when transcoding not necessary, making GStreamer's
	work very easy).

30 April 2009 W. Michael Petullo <mike@flyn.org>

	* Fix compiler warnings.

23 April 2009 W. Michael Petullo <mike@flyn.org>

	* Write GGst[Format]InputStreamFactory classes.

19 April 2009 W. Michael Petullo <mike@flyn.org>

	* Fix segfault when loading audio file that GStreamer can not
	recognize.

	* Fix segfault when loading picture that GraphicsMagick can not
	recognize.

	* Create subclasses of GGstInputStream providing for MP3,
	WAV and raw (original encoding) streams.

18 April 2009 W. Michael Petullo <mike@flyn.org>

	* Start work on user configurable transcoding format

10 April 2009 W. Michael Petullo <mike@flyn.org>

	* dmapd.c not #include's unistd.h to fix Mac OS X build.

	* Transcode using a depth and width of 8 (instead of 16) to save
	network bandwidth.

07 April 2009 W. Michael Petullo <mike@flyn.org>

	* Change to reflect libdmapsharing's use of a guint for record
	ID's.

29 March 2009 W. Michael Petullo <mike@flyn.org>

	* Fix a lot of compiler warnings.

	* Check for existence of /var/cache/dmapd.

26 March 2009 W. Michael Petullo <mike@flyn.org>

	* Fix determining format from real_format.

	* Attempt to fix a deadlock that can occur when
	g_gst_input_stream_close () is called.

24 March 2009 W. Michael Petullo <mike@flyn.org>

	* Make dmapd_dmap_container_record_get_entries return a const
	DMAPDb *.

21 March 2009 W. Michael Petullo <mike@flyn.org>

	* Limit decoded audio buffer size so that it does not exhaust
	memory to store an uncompressed song.

	* Start work on seeking transcoded streams.

20 March 2009 W. Michael Petullo <mike@flyn.org>

	* Transcode to WAV instead of MP3 to satisfy Fedora.

14 March 2009 W. Michael Petullo <mike@flyn.org>

	* Make GGstInputStream implement GSeekable.

01 March 2009 W. Michael Petullo <mike@flyn.org>

	* Fix rare timing issue that caused buffer overflow.

22 February 2009 W. Michael Petullo <mike@flyn.org>

	* Work to clean up g-gst-input-stream.c and dmapd-daap-record.c.

13 February 2009 W. Michael Petullo <mike@flyn.org>

	* Add support for FLAC.

12 February 2009 W. Michael Petullo <mike@flyn.org>

	* Migrated to new libdmapsharing API. Support Roku SoundBridge
	line.

09 February 2009 W. Michael Petullo <mike@flyn.org>

	* Fixed media type detection for audio / video.

01 February 2009 W. Michael Petullo <mike@flyn.org>

	* Implement realtime transcoding.

	* Create media tag reader pipeline only once.

	* Determine format by looking at stream.

30 January 2009 W. Michael Petullo <mike@flyn.org>

	* Fix some memory leaks.

29 January 2009 W. Michael Petullo <mike@flyn.org>

	* Read metadata from audio / video files.

28 January 2009 W. Michael Petullo <mike@flyn.org>

	* Add detection of video CODEC and set has_video.

	* Fix double call to DMAP factory create.

21 January 2009 W. Michael Petullo <mike@flyn.org>

	* Add -n, set share name.

	* Wait for child processes properly.

19 January 2009 W. Michael Petullo <mike@flyn.org>

	* Add support for has-video meta record.

	* Modify to use a record factory.

	* Allow multiple media directory roots.

	* Serve DAAP and DPAP simultaneously.

	* Work on init script.

18 January 2009 W. Michael Petullo <mike@flyn.org>

	* Modified to reflect changes to libdmapsharing API.

16 January 2009 W. Michael Petullo <mike@flyn.org>

	* Add a photo data disk cache so that metadata does not need to
	be re-read / re-generated each time you run dmapd.

	* Fork into a daemon.

	* Media ID's start at maximum and go down. Container ID's start
	at 1 and go up.

11 January 2009 W. Michael Petullo <mike@flyn.org>

	* Fix aspect ratio of thumbnails for photos that have a portrait
	orientation.

10 January 2009 W. Michael Petullo <mike@flyn.org>

	* Added album support.

28 December 2008 W. Michael Petullo <mike@flyn.org>

	* Began project.
EOF

exit 0
