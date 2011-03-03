/*
 * Audio / Video metadata functions.
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

#include "av-meta-reader.h"

static void
av_meta_reader_init (AVMetaReader *reader)
{
}

static void
av_meta_reader_class_init (AVMetaReaderClass *klass)
{
}

G_DEFINE_TYPE (AVMetaReader, av_meta_reader, G_TYPE_OBJECT)

gboolean av_meta_reader_read (AVMetaReader *reader,
			      DAAPRecord *record,
			      const gchar *path)
{
	return AV_META_READER_GET_CLASS (reader)->read (reader, record, path);
}

GOptionGroup *av_meta_reader_get_option_group (AVMetaReader *reader)
{
	return AV_META_READER_GET_CLASS (reader)->get_option_group (reader);
}
