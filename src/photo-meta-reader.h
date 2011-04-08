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
#define PHOTO_META_READER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					  TYPE_PHOTO_META_READER, \
					  PhotoMetaReaderPrivate))

typedef struct _PhotoMetaReader PhotoMetaReader;
typedef struct _PhotoMetaReaderClass PhotoMetaReaderClass;

typedef struct PhotoMetaReaderPrivate PhotoMetaReaderPrivate;

struct _PhotoMetaReader {
	GObject parent;
	PhotoMetaReaderPrivate *priv;
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
