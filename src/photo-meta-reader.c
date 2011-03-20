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
