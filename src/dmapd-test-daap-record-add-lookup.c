#include <check.h>
#include <gmodule.h>

#include "util.h"
#include "dmapd-daap-record.h"
#include "dmapd-daap-record-factory.h"
#include "dmapd-dmap-db-bdb.h"

START_TEST(test_dmapd_daap_record_add_lookup)
{
	guint id;
        guint64 filesize;
        char *location;
        char *format;
        char *title;
        char *album;
        char *artist;
        char *genre;
        gint rating;
        gint32 duration;
        gint32 track;
        gint32 year;
        gint32 firstseen;
        gint32 mtime;
        gint32 disc;
        gint32 bitrate;
        gboolean has_video;
	DmapdDAAPRecordFactory *factory;
	DMAPRecord *record1, *record2;

	factory = g_object_new (TYPE_DMAPD_DAAP_RECORD_FACTORY, NULL);

	DMAPDb *db = DMAP_DB (object_from_module (TYPE_DMAPD_DMAP_DB,
						 "ghashtable",
						 "record-factory",
						  factory,
						  NULL));

	record1 = DMAP_RECORD (
		g_object_new (TYPE_DMAPD_DAAP_RECORD, 
			      "location", "aaaaaaaaaaaaaaaaaaaa",
			      "title", "b",
			      "songalbum", "c",
			      "songartist", "d",
			      "songgenre", "e",
			      "format", "f",
			      "rating", 1,
			      "filesize", 2L,
			      "duration", 3,
			      "track", 4,
			      "year", 5,
			      "firstseen", 6,
			      "mtime", 7,
			      "disc", 8,
			      "bitrate", 9,
			      "has-video", TRUE,
			      NULL));

	id = dmap_db_add (db, record1);

	record2 = dmap_db_lookup_by_id (db, id);

	g_object_get (record2, "location", &location,
			       "title", &title,
			       "songalbum", &album,
			       "songartist", &artist,
			       "songgenre", &genre,
			       "format", &format,
			       "rating", &rating,
			       "filesize", &filesize,
			       "duration", &duration,
			       "track", &track,
			       "year", &year,
			       "firstseen", &firstseen,
			       "mtime", &mtime,
			       "disc", &disc,
			       "bitrate", &bitrate,
			       "has-video", &has_video,
			       NULL);

	fail_unless (! strcmp (location,    "aaaaaaaaaaaaaaaaaaaa"));
	fail_unless (! strcmp (title,       "b"));
	fail_unless (! strcmp (album,       "c"));
	fail_unless (! strcmp (artist,      "d"));
	fail_unless (! strcmp (genre,       "e"));
	fail_unless (! strcmp (format,      "f"));
	fail_unless (rating    == 1);
	fail_unless (filesize  == 2);
	fail_unless (duration  == 3);
	fail_unless (track     == 4);
	fail_unless (year      == 5);
	fail_unless (firstseen == 6);
	fail_unless (mtime     == 7);
	fail_unless (disc      == 8);
	fail_unless (bitrate   == 9);
	fail_unless (has_video == TRUE);

	g_object_unref (db);
	g_object_unref (factory);
}
END_TEST
