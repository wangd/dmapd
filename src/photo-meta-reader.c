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

#include "photo-meta-reader.h"

static void
photo_meta_reader_init (PhotoMetaReader *reader)
{
}

static void
photo_meta_reader_class_init (PhotoMetaReaderClass *klass)
{
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
