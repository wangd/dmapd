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

#ifndef __PHOTO_META_READER
#define __PHOTO_META_READER

#include <glib.h>
#include <libdmapsharing/dmap.h>

G_BEGIN_DECLS

#define TYPE_PHOTO_META_READER          (photo_meta_reader_get_type ())
#define PHOTO_META_READER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_PHOTO_META_READER, PhotoMetaReader))
#define PHOTO_META_READER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), \
                                         TYPE_PHOTO_META_READER, \
                                         PhotoMetaReaderClass))
#define IS_PHOTO_META_READER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                         TYPE_PHOTO_META_READER))
#define IS_PHOTO_META_READER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                         TYPE_PHOTO_META_READER))
#define PHOTO_META_READER_GET_CLASS(k) (G_TYPE_INSTANCE_GET_CLASS ((k), \
                                        TYPE_PHOTO_META_READER, \
                                        PhotoMetaReaderClass))

typedef struct _PhotoMetaReader PhotoMetaReader;
typedef struct _PhotoMetaReaderClass PhotoMetaReaderClass;

struct _PhotoMetaReader {
	GObject parent;
};

struct _PhotoMetaReaderClass {
        GObjectClass parent;

	gboolean      (*read) (PhotoMetaReader *reader,
                               DPAPRecord *record,
                               const gchar *path);
	GOptionGroup *(*get_option_group) (PhotoMetaReader *reader);
};

GType       photo_meta_reader_get_type      (void);

gboolean photo_meta_reader_read (PhotoMetaReader *reader,
                                 DPAPRecord *record,
                                 const gchar *path);

GOptionGroup *photo_meta_reader_get_option_group (PhotoMetaReader *reader);

#endif /* __PHOTO_META_READER */

G_END_DECLS
