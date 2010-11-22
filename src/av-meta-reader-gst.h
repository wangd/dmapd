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

#ifndef __AV_META_READER_GST
#define __AV_META_READER_GST

#include <glib.h>
#include <gst/gst.h>

#include "av-meta-reader.h"

G_BEGIN_DECLS

#define TYPE_AV_META_READER_GST          (av_meta_reader_gst_get_type ())
#define AV_META_READER_GST(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_AV_META_READER_GST, AVMetaReaderGst))
#define AV_META_READER_GST_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), \
                                      TYPE_AV_META_READER_GST, AVMetaReaderGstClass))
#define IS_AV_META_READER_GST(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                      TYPE_AV_META_READER_GST))
#define IS_AV_META_READER_GST_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                      TYPE_AV_META_READER_GST_CLASS))
#define AV_META_READER_GST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
                                      TYPE_AV_META_READER_GST, AVMetaReaderGstClass))
#define AV_META_READER_GST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                      TYPE_AV_META_READER_GST, AVMetaReaderGstPrivate))

typedef struct AVMetaReaderGstPrivate AVMetaReaderGstPrivate;

typedef struct {
        AVMetaReader parent;
	AVMetaReaderGstPrivate *priv;
} AVMetaReaderGst;

typedef struct {
        AVMetaReaderClass parent;
} AVMetaReaderGstClass;

GType       av_meta_reader_gst_get_type      (void);

#endif /* __AV_META_READER_GST */

G_END_DECLS
