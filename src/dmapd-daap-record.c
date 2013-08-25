/*
 *  Database record class for DAAP sharing
 *
 *  Copyright (C) 2008 W. Michael Petullo <mike@flyn.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dmapd-daap-record.h"
#include "av-meta-reader.h"
#include "util.h"

static const char *unknown = "Unknown";

struct DmapdDAAPRecordPrivate {
	char *location;
	GByteArray *hash;
	guint64 filesize;
	const char *format;	 	/* Format, possibly after transcoding. */
	gint mediakind;
	char *title;
	const char *album;
	const char *sort_album;
	const char *artist;
	const char *sort_artist;
	const char *genre;
	gboolean has_video;
	gint rating;
	gint32 duration;
	gint32 track;
	gint32 year;
	gint32 firstseen;
	gint32 mtime;
	gint32 disc;
	gint32 bitrate;
};

enum {
	PROP_0,
	PROP_LOCATION,
	PROP_HASH,
	PROP_TITLE,
	PROP_RATING,
	PROP_FILESIZE,
	PROP_ALBUM,
	PROP_SORT_ALBUM,
	PROP_ARTIST,
	PROP_SORT_ARTIST,
	PROP_GENRE,
	PROP_FORMAT,
	PROP_MEDIAKIND,
	PROP_DURATION,
	PROP_TRACK,
	PROP_YEAR,
	PROP_FIRSTSEEN,
	PROP_MTIME,
	PROP_DISC,
	PROP_BITRATE,
	PROP_HAS_VIDEO
};

static void
dmapd_daap_record_set_property (GObject *object,
				guint prop_id,
				const GValue *value,
				GParamSpec *pspec)
{
	DmapdDAAPRecord *record = DMAPD_DAAP_RECORD (object);

	switch (prop_id) {
		const char *str;
		case PROP_LOCATION:
			g_free (record->priv->location);
			record->priv->location = g_value_dup_string (value);
			break;
		case PROP_HASH:
                        if (record->priv->hash) {
                                g_byte_array_unref (record->priv->hash);
                        }
                        record->priv->hash = g_byte_array_ref (g_value_get_pointer (value));
                        break;
		case PROP_TITLE:
			g_free (record->priv->title);
			record->priv->title = g_value_dup_string (value);
			break;
		case PROP_ALBUM:
			stringleton_unref (record->priv->album);
			record->priv->album = stringleton_ref (g_value_get_string(value));
			break;
		case PROP_SORT_ALBUM:
			str = g_value_get_string(value);
			stringleton_unref (record->priv->sort_album);
			record->priv->sort_album = str ? stringleton_ref (str) : NULL;
			break;
		case PROP_ARTIST:
			stringleton_unref (record->priv->artist);
			record->priv->artist = stringleton_ref (g_value_get_string(value));
			break;
		case PROP_SORT_ARTIST:
			str = g_value_get_string(value);
			stringleton_unref (record->priv->sort_artist);
			record->priv->sort_artist = str ? stringleton_ref (str) : NULL;
			break;
		case PROP_GENRE:
			stringleton_unref (record->priv->genre);
			record->priv->genre = stringleton_ref (g_value_get_string(value));
			break;
		case PROP_FORMAT:
			stringleton_unref (record->priv->format);
			record->priv->format = stringleton_ref (g_value_get_string(value));
			break;
		case PROP_MEDIAKIND:
			record->priv->mediakind = g_value_get_enum (value);
			break;
		case PROP_RATING:
			record->priv->rating = g_value_get_int (value);
			break;
		case PROP_FILESIZE:
			record->priv->filesize = g_value_get_uint64 (value);
			break;
		case PROP_DURATION:
			record->priv->duration = g_value_get_int (value);
			break;
		case PROP_TRACK:
			record->priv->track = g_value_get_int (value);
			break;
		case PROP_YEAR:
			record->priv->year = g_value_get_int (value);
			break;
		case PROP_FIRSTSEEN:
			record->priv->firstseen = g_value_get_int (value);
			break;
		case PROP_MTIME:
			record->priv->mtime = g_value_get_int (value);
			break;
		case PROP_DISC:
			record->priv->disc = g_value_get_int (value);
			break;
		case PROP_BITRATE:
			record->priv->bitrate = g_value_get_int (value);
			break;
		case PROP_HAS_VIDEO:
			record->priv->has_video = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
							   prop_id,
							   pspec);
			break;
	}
}

static void
dmapd_daap_record_get_property (GObject *object,
				guint prop_id,
				GValue *value,
				GParamSpec *pspec)
{
	DmapdDAAPRecord *record = DMAPD_DAAP_RECORD (object);

	switch (prop_id) {
		case PROP_LOCATION:
			g_value_set_static_string (value, record->priv->location);
			break;
		case PROP_HASH:
                        g_value_set_pointer (value, record->priv->hash);
                        break;
		case PROP_TITLE:
			g_value_set_static_string (value, record->priv->title);
			break;
		case PROP_ALBUM:
			g_value_set_static_string (value, record->priv->album);
			break;
		case PROP_SORT_ALBUM:
			g_value_set_static_string (value, record->priv->sort_album);
			break;
		case PROP_ARTIST:
			g_value_set_static_string (value, record->priv->artist);
			break;
		case PROP_SORT_ARTIST:
			g_value_set_static_string (value, record->priv->sort_artist);
			break;
		case PROP_GENRE:
			g_value_set_static_string (value, record->priv->genre);
			break;
		case PROP_FORMAT:
			g_value_set_static_string (value, record->priv->format);
			break;
		case PROP_MEDIAKIND:
			g_value_set_enum (value, record->priv->mediakind);
			break;
		case PROP_RATING:
			g_value_set_int (value, record->priv->rating);
			break;
		case PROP_FILESIZE:
			g_value_set_uint64 (value, record->priv->filesize);
			break;
		case PROP_DURATION:
			g_value_set_int (value, record->priv->duration);
			break;
		case PROP_TRACK:
			g_value_set_int (value, record->priv->track);
			break;
		case PROP_YEAR:
			g_value_set_int (value, record->priv->year);
			break;
		case PROP_FIRSTSEEN:
			g_value_set_int (value, record->priv->firstseen);
			break;
		case PROP_MTIME:
			g_value_set_int (value, record->priv->mtime);
			break;
		case PROP_DISC:
			g_value_set_int (value, record->priv->disc);
			break;
		case PROP_BITRATE:
			g_value_set_int (value, record->priv->bitrate);
			break;
		case PROP_HAS_VIDEO:
			g_value_set_boolean (value, record->priv->has_video);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
							   prop_id,
							   pspec);
			break;
	}
}

gboolean dmapd_daap_record_itunes_compat (DAAPRecord *record)
{
	const gchar *format = DMAPD_DAAP_RECORD (record)->priv->format;

	if (! strcmp (format, "mp3"))
		return TRUE;
	else
		return FALSE;
}

GInputStream *dmapd_daap_record_read (DAAPRecord *record, GError **error)
{
	GFile *file;
	GInputStream *fnval = NULL;

	file = g_file_new_for_uri (DMAPD_DAAP_RECORD (record)->priv->location);
	g_assert (file);
	fnval = G_INPUT_STREAM (g_file_read (file, NULL, error));

	return fnval;
}

static GByteArray *
dmapd_daap_record_to_blob (DMAPRecord *record)
{
	/* FIXME: not endian safe. Don't write with one endianness and
	 * read with another
	 */
	
	DmapdDAAPRecordPrivate *priv = DMAPD_DAAP_RECORD (record)->priv;
	GByteArray *blob = g_byte_array_new ();

	/* NOTE: do not store ID in the blob. */

	g_assert (priv->location);
	g_assert (priv->hash);
	g_assert (priv->format);
	g_assert (priv->title);
	g_assert (priv->album);
	g_assert (priv->artist);
	g_assert (priv->genre);

	blob_add_string (blob, VERSION);
	blob_add_string (blob, priv->location);

	blob_add_atomic (blob, (const guint8 *) &(priv->hash->len), sizeof (priv->hash->len));
	g_byte_array_append (blob, priv->hash->data, priv->hash->len);

	blob_add_atomic   (blob, (const guint8 *) &(priv->filesize),
			   sizeof (priv->filesize));

	blob_add_string (blob, priv->format);
	blob_add_string (blob, priv->title);
	blob_add_string (blob, priv->album);
	blob_add_string (blob, priv->artist);
	blob_add_string (blob, priv->genre);

	blob_add_atomic   (blob, (const guint8 *) &(priv->has_video),
			   sizeof (priv->has_video));
	blob_add_atomic   (blob, (const guint8 *) &(priv->mediakind),
			   sizeof (priv->mediakind));
	blob_add_atomic   (blob, (const guint8 *) &(priv->rating),
			   sizeof (priv->rating));
	blob_add_atomic   (blob, (const guint8 *) &(priv->duration),
			   sizeof (priv->duration));
	blob_add_atomic   (blob, (const guint8 *) &(priv->track),
			   sizeof (priv->track));
	blob_add_atomic   (blob, (const guint8 *) &(priv->year),
			   sizeof (priv->year));
	blob_add_atomic   (blob, (const guint8 *) &(priv->firstseen),
			   sizeof (priv->firstseen));
	blob_add_atomic   (blob, (const guint8 *) &(priv->mtime),
			   sizeof (priv->mtime));
	blob_add_atomic   (blob, (const guint8 *) &(priv->disc),
			   sizeof (priv->disc));
	blob_add_atomic   (blob, (const guint8 *) &(priv->bitrate),
			   sizeof (priv->bitrate));
	
	return blob;
}

static gboolean
dmapd_daap_record_set_from_blob (DMAPRecord *_record, GByteArray *blob)
{
	gboolean fnval = FALSE;
	DmapdDAAPRecord *record = NULL;
	GFile *file = NULL;
	GFileInputStream *stream = NULL;
	GError *error = NULL;
	guint8 *ptr = blob->data;

	char *version;
	guint size;
	guint64 filesize;
	char *location;
	char *format;
	GByteArray *hash = NULL;
	guchar hash2[DMAP_HASH_SIZE];
	char *title;
	char *songalbum;
	char *songartist;
	char *songgenre;
	gboolean has_video;
	gint mediakind;
	gint rating;
	gint32 duration;
	gint32 track;
	gint32 year;
	gint32 firstseen;
	gint32 mtime;
	gint32 disc;
	gint32 bitrate;

	version = (char *) ptr;
	ptr += strlen (version) + 1;
	if (strcmp (version, VERSION)) {
		g_warning ("Cache written by wrong dmapd version");
		goto _done;
	}

	location = (char *) ptr;
        ptr += strlen (location) + 1;

	size = *((guint *) ptr);
	if (DMAP_HASH_SIZE != size) {
                g_warning ("Improper hash size in cache\n");
                goto _done;
        }
        ptr += sizeof (size);

        hash = g_byte_array_sized_new (size);
	if (NULL == hash) {
		g_warning ("Error allocating array for hash\n");
		goto _done;
	}
        g_byte_array_append (hash, ptr, size);
        ptr += size;

	filesize = *(guint64 *) ptr;
        ptr += sizeof (record->priv->filesize);

	format = (char *) ptr;
        ptr += strlen ((char *) ptr) + 1;

	title = (char *) ptr;
        ptr += strlen ((char *) ptr) + 1;

	songalbum = (char *) ptr;
        ptr += strlen ((char *) ptr) + 1;

	songartist = (char *) ptr;
        ptr += strlen ((char *) ptr) + 1;

	songgenre = (char *) ptr;
        ptr += strlen ((char *) ptr) + 1;

	has_video = *(gint *) ptr;
        ptr += sizeof (record->priv->has_video);

	mediakind = *(gint *) ptr;
	ptr += sizeof (record->priv->mediakind);

	rating = *(gint *) ptr;
        ptr += sizeof (record->priv->rating);

	duration = *(gint32 *) ptr;
        ptr += sizeof (record->priv->duration );

	track = *(gint32 *) ptr;
        ptr += sizeof (record->priv->track);

	year = *(gint32 *) ptr;
        ptr += sizeof (record->priv->year);

	firstseen = *(gint32 *) ptr;
        ptr += sizeof (record->priv->firstseen);

	mtime = *(gint32 *) ptr;
        ptr += sizeof (record->priv->mtime);

	disc = *(gint32 *) ptr;
        ptr += sizeof (record->priv->disc);

	bitrate = *(gint32 *) ptr;
        ptr += sizeof (record->priv->bitrate);

	file = g_file_new_for_uri (location);
        if (NULL == location) {
                g_warning ("Could not open %s\n", location);
                goto _done;
        }	

	stream = g_file_read (file, NULL, &error);
	if (NULL == stream) {
                g_warning ("Could not read %s: %s\n", location, error->message);
                goto _done;
	}

	if (! dmapd_util_hash_file (location, hash2)
         || memcmp (hash->data, hash2, DMAP_HASH_SIZE)) {
		g_warning ("Media file has changed since being cached\n");
		goto _done;
        }

	record = DMAPD_DAAP_RECORD (_record);

	g_object_set (record, "year", year,
	                      "filesize", filesize,
	                      "location", location,
                              "hash", hash,
	                      "format", format,
	                      "title", title,
	                      "songalbum", songalbum,
	                      "songartist", songartist,
	                      "songgenre", songgenre,
	                      "has-video", has_video,
	                      "mediakind", mediakind,
	                      "rating", rating,
	                      "duration", duration,
	                      "track", track,
	                      "firstseen", firstseen,
	                      "mtime", mtime,
	                      "disc", disc,
	                      "bitrate", bitrate, NULL);

	fnval = TRUE;

_done:
	if (NULL != file) {
		g_object_unref (file);
	}

	if (NULL != stream) {
		g_object_unref (stream);
	}

	if (NULL != hash) {
		g_byte_array_unref (hash);
	}

	return fnval;
}

static void dmapd_daap_record_init (DmapdDAAPRecord *record)
{
	record->priv = DMAPD_DAAP_RECORD_GET_PRIVATE (record);
}

static void dmapd_daap_record_finalize (GObject *object);

static void dmapd_daap_record_class_init (DmapdDAAPRecordClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DmapdDAAPRecordPrivate));

	gobject_class->set_property = dmapd_daap_record_set_property;	
	gobject_class->get_property = dmapd_daap_record_get_property;	
	gobject_class->finalize     = dmapd_daap_record_finalize;	

	g_object_class_override_property (gobject_class, PROP_LOCATION, "location");
	g_object_class_override_property (gobject_class, PROP_HASH, "hash");
	g_object_class_override_property (gobject_class, PROP_TITLE, "title");
	g_object_class_override_property (gobject_class, PROP_ALBUM, "songalbum");
	g_object_class_override_property (gobject_class, PROP_SORT_ALBUM, "sort-album");
	g_object_class_override_property (gobject_class, PROP_ARTIST, "songartist");
	g_object_class_override_property (gobject_class, PROP_SORT_ARTIST, "sort-artist");
	g_object_class_override_property (gobject_class, PROP_GENRE, "songgenre");
	g_object_class_override_property (gobject_class, PROP_FORMAT, "format");
	g_object_class_override_property (gobject_class, PROP_RATING, "rating");
	g_object_class_override_property (gobject_class, PROP_FILESIZE, "filesize");
	g_object_class_override_property (gobject_class, PROP_DURATION, "duration");
	g_object_class_override_property (gobject_class, PROP_TRACK, "track");
	g_object_class_override_property (gobject_class, PROP_YEAR, "year");
	g_object_class_override_property (gobject_class, PROP_FIRSTSEEN, "firstseen");
	g_object_class_override_property (gobject_class, PROP_MTIME, "mtime");
	g_object_class_override_property (gobject_class, PROP_DISC, "disc");
	g_object_class_override_property (gobject_class, PROP_BITRATE, "bitrate");
	g_object_class_override_property (gobject_class, PROP_HAS_VIDEO, "has-video");
	g_object_class_override_property (gobject_class, PROP_MEDIAKIND, "mediakind");
}

static void dmapd_daap_record_daap_iface_init (gpointer iface, gpointer data)
{
	DAAPRecordIface *daap_record = iface;

	g_assert (G_TYPE_FROM_INTERFACE (daap_record) == DAAP_TYPE_RECORD);

	daap_record->itunes_compat = dmapd_daap_record_itunes_compat;
	daap_record->read = dmapd_daap_record_read;
}

static void dmapd_daap_record_dmap_iface_init (gpointer iface, gpointer data)
{
	DMAPRecordIface *dmap_record = iface;

	g_assert (G_TYPE_FROM_INTERFACE (dmap_record) == DMAP_TYPE_RECORD);

	dmap_record->to_blob = dmapd_daap_record_to_blob;
	dmap_record->set_from_blob = dmapd_daap_record_set_from_blob;
}


G_DEFINE_TYPE_WITH_CODE (DmapdDAAPRecord, dmapd_daap_record, G_TYPE_OBJECT, 
			 G_IMPLEMENT_INTERFACE (DAAP_TYPE_RECORD, dmapd_daap_record_daap_iface_init)
			 G_IMPLEMENT_INTERFACE (DMAP_TYPE_RECORD, dmapd_daap_record_dmap_iface_init))

static void
dmapd_daap_record_finalize (GObject *object)
{
	DmapdDAAPRecord *record = DMAPD_DAAP_RECORD (object);

	g_debug ("Free'ing record");

	g_free (record->priv->location);
	g_free (record->priv->title);

	stringleton_unref (record->priv->format);
	stringleton_unref (record->priv->album);
	stringleton_unref (record->priv->artist);
	stringleton_unref (record->priv->genre);

	G_OBJECT_CLASS (dmapd_daap_record_parent_class)->finalize (object);
}

DmapdDAAPRecord *dmapd_daap_record_new (const char *path, AVMetaReader *reader)
{
	DmapdDAAPRecord *record = NULL;
	guchar hash_buf[DMAP_HASH_SIZE];
	struct stat buf;
	char *title = NULL;
	char *location = NULL;
	GByteArray *hash = NULL;

	if (path) {
		location = g_filename_to_uri (path, NULL, NULL);
		if (NULL == location) {
			g_warning ("Error converting %s to URI\n", path);
			goto _done;
		}

		title = g_path_get_basename (path);
		if (NULL == title) {
			g_warning ("Error extracting filename from %s\n", path);
			goto _done;
		}

		hash = g_byte_array_sized_new (DMAP_HASH_SIZE);
		if (NULL == hash) {
			g_warning ("Error allocating memory for record's hash field\n");
			goto _done;
		}

		if (! dmapd_util_hash_file (location, hash_buf)) {
			g_warning ("Unable to hash %s\n", location);
			goto _done;
		}

		g_byte_array_append (hash, hash_buf, DMAP_HASH_SIZE);

		record = DMAPD_DAAP_RECORD (g_object_new (TYPE_DMAPD_DAAP_RECORD, NULL));
		if (NULL == record) {
			g_warning ("Error allocating memory for record\n");
			goto _done;
		}

		if (stat (path, &buf) == -1) {
			g_warning ("Unable to determine size of %s", path);
		} else {
			g_object_set (record,
			             "filesize",
				     (guint64) buf.st_size,
				     "mtime",
				     (guint64) buf.st_mtime,
				      NULL);
		}

		g_object_set (record, "location",    location,
		                      "hash",        hash,
		                      "title",       title,
		                      "songartist",  unknown,
		                      "songalbum",   unknown,
		                      "songgenre",   unknown,
		                      "format",      unknown,
		                      "mediakind",   DMAP_MEDIA_KIND_MUSIC,
		                      "year",        1985,
		                      "disc",        1, NULL);

		if (! av_meta_reader_read (AV_META_READER (reader), DAAP_RECORD (record), path)) {
			g_object_unref (record);
			record = NULL;
			goto _done;
		}

		record->priv->rating = 5;	/* FIXME */
		record->priv->firstseen = 1;	/* FIXME */
		record->priv->bitrate = 128;	/* FIXME, from codec decoder */
	} else {
		record = DMAPD_DAAP_RECORD (g_object_new (TYPE_DMAPD_DAAP_RECORD, NULL));
		if (NULL == record) {
			g_warning ("Error allocating memory for record");
			goto _done;
		}
	}

_done:

	if (NULL != location) {
		g_free (location);
	}

	if (NULL != title) {
		g_free (title);
	}

	if (NULL != hash) {
		g_byte_array_unref (hash);
	}

	return record;
}
