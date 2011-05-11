/*
 * Photo metadata functions.
 *
 * Copyright (C) 2009 W. Michael Petullo <mike@flyn.org>
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
 * You should hphotoe received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wand/wand_api.h>
#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>

#include "photo-meta-reader-graphicsmagick.h"
#include "dmapd-dpap-record.h"

const int DEFAULT_MAX_THUMBNAIL_WIDTH = 128;

struct PhotoMetaReaderGraphicsmagickPrivate {
};

static void
photo_meta_reader_graphicsmagick_set_property (GObject *object,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
photo_meta_reader_graphicsmagick_get_property (GObject *object,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static GOptionGroup *
photo_meta_reader_graphicsmagick_get_option_group (PhotoMetaReader *reader)
{
        return NULL;
}

static gboolean
photo_meta_reader_graphicsmagick_read (PhotoMetaReader *reader,
				       DPAPRecord *record,
				       const gchar *path)
{
	gboolean fnval = FALSE;
	MagickWand *wand;
	wand = NewMagickWand ();

	if (! MagickReadImage (wand, path)) {
		g_warning ("Could not read %s", path);
	} else {
		//ExifData *d;
		struct stat buf;
		size_t thumbnail_size;
		guchar *thumbnail_data;
		GByteArray *thumbnail_array;
		float aspect_ratio;
		gchar *aspect_ratio_str;
		gchar *location;
		guint max_thumbnail_width = 0;

		if (stat (path, &buf) == -1) {
			g_warning ("Unable to determine size of %s", path);
		} else {
			g_object_set (record, "large-filesize", buf.st_size, NULL);
		}

		g_object_set (record, "creation-date", 1, NULL);
		g_object_set (record, "rating", 5, NULL);
		g_object_set (record, "filename", g_basename (path), NULL);

		location = g_strdup_printf ("file//%s", path);
		g_object_set (record, "location", location, NULL);
		g_free (location);

		MagickResetIterator (wand);

		g_object_set (record, "format", MagickGetImageFormat (wand), NULL);
		g_object_set (record, "pixel-height", MagickGetImageHeight (wand), NULL);
		g_object_set (record, "pixel-width", MagickGetImageWidth (wand), NULL);
		g_object_set (record, "comments", "", NULL);

		aspect_ratio = MagickGetImageWidth (wand) / (float) MagickGetImageHeight (wand);
		aspect_ratio_str = g_strdup_printf ("%f", aspect_ratio);
		g_object_set (record, "aspect-ratio", aspect_ratio_str, NULL);
		g_free (aspect_ratio_str);

		g_object_get (reader, "max-thumbnail-width", &max_thumbnail_width, NULL);
		if (! max_thumbnail_width) {
			max_thumbnail_width = DEFAULT_MAX_THUMBNAIL_WIDTH;
		}

		if (MagickGetImageWidth (wand) > MagickGetImageHeight (wand)) {
			MagickResizeImage (wand,
					   max_thumbnail_width,
					   (1 / aspect_ratio) * max_thumbnail_width,
					   LanczosFilter,
					   1.0);
		} else {
			MagickResizeImage (wand,
					   aspect_ratio * max_thumbnail_width,
					   max_thumbnail_width,
					   LanczosFilter,
					   1.0);
		}
		thumbnail_data = MagickWriteImageBlob (wand, &thumbnail_size);
		thumbnail_array = g_byte_array_sized_new (thumbnail_size);
		g_byte_array_append (thumbnail_array, thumbnail_data, thumbnail_size);
		MagickRelinquishMemory (thumbnail_data);
		g_object_set (record, "thumbnail", thumbnail_array, NULL);
		g_byte_array_unref(thumbnail_array);

		/* FIXME:
		d = exif_data_new_from_file (path);
		if (! d) {
			g_warning ("Failed to EXIF data from %s", path);
		} else {
			int i;
			for (i = 0; i < EXIF_IFD_COUNT; i++) {
				ExifContent *c = d->ifd[i];
				if (! c || ! c->count) {
					g_warning ("Failed to find EXIF content in %s", path);
				} else {
					ExifEntry *e = exif_content_get_entry (c, EXIF_TAG_USER_COMMENT);
					if (! e) {
						g_warning ("Failed to get comments EXIF entry in %s", path);
					} else {
						gchar v[BUFSIZ + 1];
						exif_entry_get_value (e, v, BUFSIZ);
						g_object_set (record, "comments", v, NULL);
					exif_content_unref (c);
					}
				}
			exif_data_unref (d);
			}
		}
		*/
		fnval = TRUE;
	}

	DestroyMagickWand (wand); 
	return fnval;
}

static void
photo_meta_reader_graphicsmagick_class_finalize (PhotoMetaReaderGraphicsmagickClass *klass)
{
}

static void photo_meta_reader_graphicsmagick_init (PhotoMetaReaderGraphicsmagick *reader)
{
	reader->priv = PHOTO_META_READER_GRAPHICSMAGICK_GET_PRIVATE (reader);
}

static void photo_meta_reader_graphicsmagick_class_init (PhotoMetaReaderGraphicsmagickClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	PhotoMetaReaderClass *photo_meta_reader = PHOTO_META_READER_CLASS (klass);

	g_type_class_add_private (klass, sizeof (PhotoMetaReaderGraphicsmagickPrivate));

	gobject_class->set_property = photo_meta_reader_graphicsmagick_set_property;
	gobject_class->get_property = photo_meta_reader_graphicsmagick_get_property;

        photo_meta_reader->read = photo_meta_reader_graphicsmagick_read;
	photo_meta_reader->get_option_group = photo_meta_reader_graphicsmagick_get_option_group;
}

static void photo_meta_reader_graphicsmagick_register_type (GTypeModule *module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule *module)
{
        photo_meta_reader_graphicsmagick_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT gboolean
dmapd_module_unload (GTypeModule *module)
{
	return FALSE;
}

G_DEFINE_DYNAMIC_TYPE (PhotoMetaReaderGraphicsmagick,
                       photo_meta_reader_graphicsmagick,
                       TYPE_PHOTO_META_READER)
