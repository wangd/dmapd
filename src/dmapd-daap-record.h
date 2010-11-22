/*
 *  Database record class for DAAP sharing
 *
 *  Copyright (C) 2008 W. Michael Petullo <mike@flyn.org>
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

#ifndef __DMAPD_DAAP_RECORD
#define __DMAPD_DAAP_RECORD

#include <libdmapsharing/dmap.h>

G_BEGIN_DECLS

#define TYPE_DMAPD_DAAP_RECORD         (dmapd_daap_record_get_type ())
#define DMAPD_DAAP_RECORD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_DMAPD_DAAP_RECORD, DmapdDAAPRecord))
#define DMAPD_DAAP_RECORD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_DMAPD_DAAP_RECORD, DmapdDAAPRecordClass))
#define IS_DMAPD_DAAP_RECORD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_DMAPD_DAAP_RECORD))
#define IS_DMAPD_DAAP_RECORD_CLASS (k) (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_DMAPD_DAAP_RECORD_CLASS))
#define DMAPD_DAAP_RECORD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_DMAPD_DAAP_RECORD, DmapdDAAPRecordClass))
#define DMAPD_DAAP_RECORD_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_DMAPD_DAAP_RECORD, DmapdDAAPRecordPrivate))

typedef struct DmapdDAAPRecordPrivate DmapdDAAPRecordPrivate;

typedef struct {
	GObject parent;
	DmapdDAAPRecordPrivate *priv;
} DmapdDAAPRecord;

typedef struct {
	GObjectClass parent;
} DmapdDAAPRecordClass;

GType dmapd_daap_record_get_type (void);

DmapdDAAPRecord *dmapd_daap_record_new (const char *location, gpointer reader);

/* FIXME: Should take AVMetaReader, but we have #include loop. */
DmapdDAAPRecord *dmapd_daap_record_new (const char *location, gpointer reader);

gint          dmapd_daap_record_get_id          (DAAPRecord *record);

gboolean      dmapd_daap_record_itunes_compat   (DAAPRecord *record);

GInputStream *dmapd_daap_record_read            (DAAPRecord *record,
						 GError **err);

#endif /* __DMAPD_DAAP_RECORD */

G_END_DECLS
