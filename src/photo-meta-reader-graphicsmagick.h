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

#ifndef __PHOTO_META_READER_GRAPHICSMAGICK
#define __PHOTO_META_READER_GRAPHICSMAGICK

#include <glib.h>

#include "photo-meta-reader.h"

G_BEGIN_DECLS

#define TYPE_PHOTO_META_READER_GRAPHICSMAGICK          (photo_meta_reader_graphicsmagick_get_type ())
#define PHOTO_META_READER_GRAPHICSMAGICK(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_PHOTO_META_READER_GRAPHICSMAGICK, PhotoMetaReaderGraphicsmagick))
#define PHOTO_META_READER_GRAPHICSMAGICK_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), \
                                      TYPE_PHOTO_META_READER_GRAPHICSMAGICK, PhotoMetaReaderGraphicsmagickClass))
#define IS_PHOTO_META_READER_GRAPHICSMAGICK(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                      TYPE_PHOTO_META_READER_GRAPHICSMAGICK))
#define IS_PHOTO_META_READER_GRAPHICSMAGICK_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                      TYPE_PHOTO_META_READER_GRAPHICSMAGICK_CLASS))
#define PHOTO_META_READER_GRAPHICSMAGICK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
                                      TYPE_PHOTO_META_READER_GRAPHICSMAGICK, PhotoMetaReaderGraphicsmagickClass))
#define PHOTO_META_READER_GRAPHICSMAGICK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                      TYPE_PHOTO_META_READER_GRAPHICSMAGICK, PhotoMetaReaderGraphicsmagickPrivate))

typedef struct PhotoMetaReaderGraphicsmagickPrivate PhotoMetaReaderGraphicsmagickPrivate;

typedef struct {
        PhotoMetaReader parent;
	PhotoMetaReaderGraphicsmagickPrivate *priv;
} PhotoMetaReaderGraphicsmagick;

typedef struct {
        PhotoMetaReaderClass parent;
} PhotoMetaReaderGraphicsmagickClass;

GType       photo_meta_reader_graphicsmagick_get_type      (void);

#endif /* __PHOTO_META_READER_GRAPHICSMAGICK */

G_END_DECLS
