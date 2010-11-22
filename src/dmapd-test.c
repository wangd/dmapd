/*   FILE: dmapd-test.c -- Unit tests
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 01 January 2009 
 *
 * Copyright (c) 2009 W. Michael Petullo <new@flyn.org>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <glib.h>
#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libdmapsharing/dmap.h>

#include "util.h"
#include "dmapd-daap-record.h"
#include "dmapd-daap-record-factory.h"
#include "dmapd-dmap-db-bdb.h"

void test (DMAPDb *db)
{
	guint id;
        guint64 filesize;
        char *location;
        char *format;
        char *real_format;
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

	DMAPRecord *record1 = DMAP_RECORD (
		g_object_new (TYPE_DMAPD_DAAP_RECORD, 
			      "location", "aaaaaaaaaaaaaaaaaaaa",
			      "title", "b",
			      "songalbum", "c",
			      "songartist", "d",
			      "songgenre", "e",
			      "format", "f",
			      "real_format", "g",
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
	DMAPRecord *record2;

	id = dmap_db_add (db, record1);

	record2 = dmap_db_lookup_by_id (db, id);

	g_object_get (record2, "location", &location,
			       "title", &title,
			       "songalbum", &album,
			       "songartist", &artist,
			       "songgenre", &genre,
			       "format", &format,
			       "real_format", &real_format,
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

	g_assert (! strcmp (location,    "aaaaaaaaaaaaaaaaaaaa"));
	g_assert (! strcmp (title,       "b"));
	g_assert (! strcmp (album,       "c"));
	g_assert (! strcmp (artist,      "d"));
	g_assert (! strcmp (genre,       "e"));
	g_assert (! strcmp (format,      "f"));
	g_assert (! strcmp (real_format, "g"));
	g_assert (rating    == 1);
	g_assert (filesize  == 2);
	g_assert (duration  == 3);
	g_assert (track     == 4);
	g_assert (year      == 5);
	g_assert (firstseen == 6);
	g_assert (mtime     == 7);
	g_assert (disc      == 8);
	g_assert (bitrate   == 9);
	g_assert (has_video == TRUE);
}

int main (int argc, char *argv[])
{
	int i = 1;
	DmapdDAAPRecordFactory *factory;

	g_type_init ();

	factory = g_object_new (TYPE_DMAPD_DAAP_RECORD_FACTORY, NULL);

	DMAPDb *db = DMAP_DB (object_from_module (TYPE_DMAPD_DMAP_DB,
						 "ghashtable",
						 "record-factory",
						  factory,
						  NULL));

	for (i = 1; i < 2; i++) {
		test (db);
		sleep (1);
	}

	g_object_unref (db);
	g_object_unref (factory);

	exit(EXIT_SUCCESS);
}
