/*
 * Photo metadata functions.
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

#include "photo-meta-reader.h"

struct PhotoMetaReaderPrivate {
	guint max_thumbnail_width;	
};

enum {
	PROP_0,
	PROP_MAX_THUMBNAIL_WIDTH
};

static void
photo_meta_reader_set_property (GObject * object,
                                guint prop_id,
				const GValue * value,
				GParamSpec * pspec)
{
	PhotoMetaReader *reader = PHOTO_META_READER (object);

	switch (prop_id) {
	case PROP_MAX_THUMBNAIL_WIDTH:
		reader->priv->max_thumbnail_width = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
photo_meta_reader_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
				GParamSpec *pspec)
{
	PhotoMetaReader *reader = PHOTO_META_READER (object);

	switch (prop_id) {
	case PROP_MAX_THUMBNAIL_WIDTH:
		g_value_set_uint (value, reader->priv->max_thumbnail_width);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
} 

static void
photo_meta_reader_init (PhotoMetaReader *reader)
{
	reader->priv = PHOTO_META_READER_GET_PRIVATE (reader);
}

static void
photo_meta_reader_class_init (PhotoMetaReaderClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (PhotoMetaReaderPrivate));

	gobject_class->set_property = photo_meta_reader_set_property;
	gobject_class->get_property = photo_meta_reader_get_property;

	g_object_class_install_property (gobject_class,
	                                 PROP_MAX_THUMBNAIL_WIDTH,
					 g_param_spec_uint ("max-thumbnail-width",
					                    "Maximum thumbnail width",
							    "Maximum thumbnail width",
							     1,
							     G_MAXUINT,
							     128,
							     G_PARAM_READWRITE));
}

G_DEFINE_TYPE (PhotoMetaReader, photo_meta_reader, G_TYPE_OBJECT)

gboolean photo_meta_reader_read (PhotoMetaReader *reader, DPAPRecord *record, const gchar *path)
{
	return PHOTO_META_READER_GET_CLASS (reader)->read (reader, record, path);
}

GOptionGroup *photo_meta_reader_get_option_group (PhotoMetaReader *reader)
{
	return PHOTO_META_READER_GET_CLASS (reader)->get_option_group (reader);
}
