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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __PHOTO_META_READER_VIPS
#define __PHOTO_META_READER_VIPS

#include <glib.h>

#include "photo-meta-reader.h"

G_BEGIN_DECLS

#define TYPE_PHOTO_META_READER_VIPS          (photo_meta_reader_vips_get_type ())
#define PHOTO_META_READER_VIPS(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_PHOTO_META_READER_VIPS, PhotoMetaReaderVips))
#define PHOTO_META_READER_VIPS_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), \
                                      TYPE_PHOTO_META_READER_VIPS, PhotoMetaReaderVipsClass))
#define IS_PHOTO_META_READER_VIPS(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                      TYPE_PHOTO_META_READER_VIPS))
#define IS_PHOTO_META_READER_VIPS_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                      TYPE_PHOTO_META_READER_VIPS_CLASS))
#define PHOTO_META_READER_VIPS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
                                      TYPE_PHOTO_META_READER_VIPS, PhotoMetaReaderVipsClass))
#define PHOTO_META_READER_VIPS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                      TYPE_PHOTO_META_READER_VIPS, PhotoMetaReaderVipsPrivate))

typedef struct PhotoMetaReaderVipsPrivate PhotoMetaReaderVipsPrivate;

typedef struct {
        PhotoMetaReader parent;
	PhotoMetaReaderVipsPrivate *priv;
} PhotoMetaReaderVips;

typedef struct {
        PhotoMetaReaderClass parent;
} PhotoMetaReaderVipsClass;

GType       photo_meta_reader_vips_get_type      (void);

#endif /* __PHOTO_META_READER_VIPS */

G_END_DECLS
