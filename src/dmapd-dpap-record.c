/*
 *  Database record class for DPAP sharing
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

#include <string.h>

#include "util.h"
#include "dmapd-dpap-record.h"
#include "photo-meta-reader.h"

struct DmapdDPAPRecordPrivate {
	gint largefilesize;
	gint creationdate;
	gint rating;
	char *location;
	char *filename;
	GByteArray *thumbnail;
	const char *aspectratio;
	gint height;
	gint width;
	const char *format;
	char *comments;
};

enum {
	PROP_0,
	PROP_LARGE_FILESIZE,
	PROP_CREATION_DATE,
	PROP_RATING,
	PROP_LOCATION,
	PROP_FILENAME,
	PROP_ASPECT_RATIO,
	PROP_PIXEL_HEIGHT,
	PROP_PIXEL_WIDTH,
	PROP_FORMAT,
	PROP_COMMENTS,
	PROP_THUMBNAIL
};

static void
dmapd_dpap_record_set_property (GObject *object,
				guint prop_id,
				const GValue *value,
				GParamSpec *pspec)
{
	DmapdDPAPRecord *record = DMAPD_DPAP_RECORD (object);

	switch (prop_id) {
		case PROP_LARGE_FILESIZE:
			record->priv->largefilesize = g_value_get_int (value);
			break;
		case PROP_CREATION_DATE:
			record->priv->creationdate = g_value_get_int (value);
			break;
		case PROP_RATING:
			record->priv->rating = g_value_get_int (value);
			break;
		case PROP_LOCATION:
			g_free (record->priv->location);
			record->priv->location = g_value_dup_string (value);
			break;
		case PROP_FILENAME:
			g_free (record->priv->filename);
			record->priv->filename = g_value_dup_string (value);
			break;
		case PROP_ASPECT_RATIO:
			stringleton_unref (record->priv->aspectratio);
			record->priv->aspectratio = stringleton_ref (g_value_get_string (value));
			break;
		case PROP_PIXEL_HEIGHT:
			record->priv->height = g_value_get_int (value);
			break;
		case PROP_PIXEL_WIDTH:
			record->priv->width = g_value_get_int (value);
			break;
		case PROP_FORMAT:
			stringleton_unref (record->priv->format);
			record->priv->format = stringleton_ref (g_value_get_string (value));
			break;
		case PROP_COMMENTS:
			g_free (record->priv->comments);
			record->priv->comments = g_value_dup_string (value);
			break;
		case PROP_THUMBNAIL:
			if (record->priv->thumbnail) {
				g_byte_array_unref (record->priv->thumbnail);
			}
			record->priv->thumbnail = g_byte_array_ref (g_value_get_pointer (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;

	}
}

static void
dmapd_dpap_record_get_property (GObject *object,
                                guint prop_id,
				GValue *value,
				GParamSpec *pspec)
{
	DmapdDPAPRecord *record = DMAPD_DPAP_RECORD (object);

	switch (prop_id) {
		case PROP_LARGE_FILESIZE:
			g_value_set_int (value, record->priv->largefilesize);
			break;
		case PROP_CREATION_DATE:
			g_value_set_int (value, record->priv->creationdate);
			break;
		case PROP_RATING:
			g_value_set_int (value, record->priv->rating);
			break;
		case PROP_LOCATION:
			g_value_set_static_string (value, record->priv->location);
			break;
		case PROP_FILENAME:
			g_value_set_static_string (value, record->priv->filename);
			break;
		case PROP_ASPECT_RATIO:
			g_value_set_static_string (value, record->priv->aspectratio);
			break;
		case PROP_PIXEL_HEIGHT:
			g_value_set_int (value, record->priv->height);
			break;
		case PROP_PIXEL_WIDTH:
			g_value_set_int (value, record->priv->width);
			break;
		case PROP_FORMAT:
			g_value_set_static_string (value, record->priv->format);
			break;
		case PROP_COMMENTS:
			g_value_set_static_string (value, record->priv->comments);
			break;
		case PROP_THUMBNAIL:
			g_value_set_pointer (value, record->priv->thumbnail);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;

	}
}

GInputStream *dmapd_dpap_record_read (DPAPRecord *record, GError **error)
{
        GFile *file;
        GInputStream *stream;

        file = g_file_new_for_uri (DMAPD_DPAP_RECORD (record)->priv->location);
        stream = G_INPUT_STREAM (g_file_read (file, NULL, error));

        g_object_unref (file);

        return stream;
}

static GByteArray *
dmapd_dpap_record_to_blob (DMAPRecord *record)
{
	/* FIXME: not endian safe. Don't write with one endianness and 
	 * read with another
	 */

	DmapdDPAPRecordPrivate *priv = DMAPD_DPAP_RECORD (record)->priv;
	GByteArray *blob = g_byte_array_new ();

	/* NOTE: do not store ID in the blob. */

	blob_add_atomic (blob, (const guint8 *) &(priv->largefilesize),
			 sizeof (priv->largefilesize));
	blob_add_atomic (blob, (const guint8 *) &(priv->creationdate),
			 sizeof (priv->creationdate));
	blob_add_atomic (blob, (const guint8 *) &(priv->rating),
			 sizeof (priv->rating));
        blob_add_string (blob, priv->location);
        blob_add_string (blob, priv->filename);
	if (priv->thumbnail) {
		blob_add_atomic (blob, (const guint8 *) &(priv->thumbnail->len),
				 sizeof (priv->thumbnail->len));

		g_byte_array_append (blob,
				     DMAPD_DPAP_RECORD (record)->priv->thumbnail->data,
				     DMAPD_DPAP_RECORD (record)->priv->thumbnail->len);
	} else {
		gsize zero = 0;
		blob_add_atomic (blob, (const guint8 *) &zero,
				 sizeof (priv->thumbnail->len));
	}

        blob_add_string (blob, priv->aspectratio);
	blob_add_atomic (blob, (const guint8 *) &(priv->height),
			 sizeof (priv->height));
	blob_add_atomic (blob, (const guint8 *) &(priv->width),
			 sizeof (priv->width));
        blob_add_string (blob, priv->format);
        blob_add_string (blob, priv->comments);

	return blob;
}

DMAPRecord *
dmapd_dpap_record_set_from_blob (DMAPRecord *_record, GByteArray *blob)
{
	guint size;
	guint8 *ptr = blob->data;
	DmapdDPAPRecord *record = DMAPD_DPAP_RECORD (_record);

	g_object_set (record, "large-filesize", *((gint *) ptr), NULL);
	ptr += sizeof (record->priv->largefilesize);

	g_object_set (record, "creation-date", *((gint *) ptr), NULL);
	ptr += sizeof (record->priv->creationdate);

	g_object_set (record, "rating", *((gint *) ptr), NULL);
	ptr += sizeof (record->priv->rating);

	g_object_set (record, "location", (char *) ptr, NULL);
	ptr += strlen ((char *) ptr) + 1;

	g_object_set (record, "filename", (char *) ptr, NULL);
	ptr += strlen ((char *) ptr) + 1;

	/* FIXME: use g_object_set: */
	size = *((guint *) ptr);
	ptr += sizeof (size);

	/* FIXME: use g_object_set?: */
	if (size) {
		record->priv->thumbnail = g_byte_array_sized_new (size);
		g_byte_array_append (record->priv->thumbnail, ptr, size);
		ptr += size;
	} else {
		record->priv->thumbnail = NULL;
	}

	g_object_set (record, "aspect-ratio", (char *) ptr, NULL);
	ptr += strlen ((char *) ptr) + 1;

	g_object_set (record, "pixel-height", *((gint *) ptr), NULL);
	ptr += sizeof (record->priv->height);

	g_object_set (record, "pixel-width", *((gint *) ptr), NULL);
	ptr += sizeof (record->priv->width);

	g_object_set (record, "format", (char *) ptr, NULL);
	ptr += strlen ((char *) ptr) + 1;

	g_object_set (record, "comments", (char *) ptr, NULL);
	ptr += strlen ((char *) ptr) + 1;

	return DMAP_RECORD (record);
}

static void dmapd_dpap_record_init (DmapdDPAPRecord *record)
{
	record->priv = DMAPD_DPAP_RECORD_GET_PRIVATE (record);
}

static void dmapd_dpap_record_finalize (GObject *object);

static void dmapd_dpap_record_class_init (DmapdDPAPRecordClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DmapdDPAPRecordPrivate));

	gobject_class->set_property = dmapd_dpap_record_set_property;
	gobject_class->get_property = dmapd_dpap_record_get_property;
	gobject_class->finalize = dmapd_dpap_record_finalize;

	g_object_class_override_property (gobject_class, PROP_LARGE_FILESIZE, "large-filesize");
	g_object_class_override_property (gobject_class, PROP_CREATION_DATE, "creation-date");
	g_object_class_override_property (gobject_class, PROP_RATING, "rating");
	g_object_class_override_property (gobject_class, PROP_LOCATION, "location");
	g_object_class_override_property (gobject_class, PROP_FILENAME, "filename");
	g_object_class_override_property (gobject_class, PROP_ASPECT_RATIO, "aspect-ratio");
	g_object_class_override_property (gobject_class, PROP_PIXEL_HEIGHT, "pixel-height");
	g_object_class_override_property (gobject_class, PROP_PIXEL_WIDTH, "pixel-width");
	g_object_class_override_property (gobject_class, PROP_FORMAT, "format");
	g_object_class_override_property (gobject_class, PROP_COMMENTS, "comments");
	g_object_class_override_property (gobject_class, PROP_THUMBNAIL, "thumbnail");
}

static void dmapd_dpap_record_dpap_iface_init (gpointer iface, gpointer data)
{
	DPAPRecordIface *dpap_record = iface;

	g_assert (G_TYPE_FROM_INTERFACE (dpap_record) == DPAP_TYPE_RECORD);

	dpap_record->read = dmapd_dpap_record_read;
}

static void dmapd_dpap_record_dmap_iface_init (gpointer iface, gpointer data)
{
	DMAPRecordIface *dmap_record = iface;

	g_assert (G_TYPE_FROM_INTERFACE (dmap_record) == DMAP_TYPE_RECORD);

	dmap_record->to_blob = dmapd_dpap_record_to_blob;
	dmap_record->set_from_blob = dmapd_dpap_record_set_from_blob;
}


G_DEFINE_TYPE_WITH_CODE (DmapdDPAPRecord, dmapd_dpap_record, G_TYPE_OBJECT, 
			 G_IMPLEMENT_INTERFACE (DPAP_TYPE_RECORD, dmapd_dpap_record_dpap_iface_init)
			 G_IMPLEMENT_INTERFACE (DMAP_TYPE_RECORD, dmapd_dpap_record_dmap_iface_init))

static void
dmapd_dpap_record_finalize (GObject *object)
{
        DmapdDPAPRecord *record = DMAPD_DPAP_RECORD (object);

	g_debug ("Free'ing record");

	stringleton_unref (record->priv->aspectratio);
	stringleton_unref (record->priv->format);

	g_free (record->priv->location);
	g_free (record->priv->filename);
	g_free (record->priv->comments);

	if (record->priv->thumbnail) {
		g_byte_array_unref (record->priv->thumbnail);
	}

	G_OBJECT_CLASS (dmapd_dpap_record_parent_class)->finalize (object);
}

DmapdDPAPRecord *dmapd_dpap_record_new (const char *path, gpointer reader)
{
	DmapdDPAPRecord *record;

	record = DMAPD_DPAP_RECORD (g_object_new (TYPE_DMAPD_DPAP_RECORD, NULL));

	/* FIXME: where can this go (what is default value for pointer props?) */
	record->priv->thumbnail = NULL;

	if (path) {
		if (! photo_meta_reader_read (PHOTO_META_READER (reader), DPAP_RECORD (record), path)) {
			g_object_unref (record);
			record = NULL;
		}
	}

	return record;
}
