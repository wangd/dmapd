/*
 * Audio / Video metadata functions.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __AV_META_READER
#define __AV_META_READER

#include <libdmapsharing/dmap.h>

G_BEGIN_DECLS

#define TYPE_AV_META_READER          (av_meta_reader_get_type ())
#define AV_META_READER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_AV_META_READER, AVMetaReader))
#define AV_META_READER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), \
                                      TYPE_AV_META_READER, AVMetaReaderClass))
#define IS_AV_META_READER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                      TYPE_AV_META_READER))
#define IS_AV_META_READER_CLASS(k)   (G_TYPE_CHECK_INSTANCE_TYPE ((k), \
                                      TYPE_AV_META_READER))
#define AV_META_READER_GET_CLASS(k)  (G_TYPE_INSTANCE_GET_CLASS ((k), \
                                      TYPE_AV_META_READER, AVMetaReaderClass))

typedef struct _AVMetaReader AVMetaReader;
typedef struct _AVMetaReaderClass AVMetaReaderClass;

struct _AVMetaReader {
	GObject parent;
};

struct _AVMetaReaderClass {
        GObjectClass parent;

	gboolean      (*read)		  (AVMetaReader *reader,
					   DAAPRecord *record,
					   const gchar *path);
	GOptionGroup *(*get_option_group) (AVMetaReader *reader);
};

GType       av_meta_reader_get_type      (void);

gboolean av_meta_reader_read (AVMetaReader    *reader,
			      DAAPRecord *record,
			      const gchar     *path);

GOptionGroup *av_meta_reader_get_option_group (AVMetaReader *reader);

#endif /* __AV_META_READER */

G_END_DECLS
