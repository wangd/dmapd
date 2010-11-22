/*
 * DmapdDPAPRecord factory class
 *
 * Copyright (C) 2008 W. Michael Petullo <mike@flyn.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __DMAPD_DPAP_RECORD_FACTORY
#define __DMAPD_DPAP_RECORD_FACTORY

#include <libdmapsharing/dmap.h>

G_BEGIN_DECLS

#define TYPE_DMAPD_DPAP_RECORD_FACTORY         (dmapd_dpap_record_factory_get_type ())
#define DMAPD_DPAP_RECORD_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), \
				               TYPE_DMAPD_DPAP_RECORD_FACTORY, DmapdDPAPRecordFactory))
#define DMAPD_DPAP_RECORD_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), \
				               TYPE_DMAPD_DPAP_RECORD_FACTORY, DmapdDPAPRecordFactoryClass))
#define IS_DMAPD_DPAP_RECORD_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
				               TYPE_DMAPD_DPAP_RECORD_FACTORY))
#define IS_DMAPD_DPAP_RECORD_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), \
				               TYPE_DMAPD_DPAP_RECORD_FACTORY_CLASS))
#define DMAPD_DPAP_RECORD_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
				               TYPE_DMAPD_DPAP_RECORD_FACTORY, DmapdDPAPRecordFactoryClass))
#define DMAPD_DPAP_RECORD_FACTORY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_DMAPD_DPAP_RECORD_FACTORY, DmapdDPAPRecordFactoryPrivate))

typedef struct DmapdDPAPRecordFactoryPrivate DmapdDPAPRecordFactoryPrivate;

typedef struct {
	GObject parent;
	DmapdDPAPRecordFactoryPrivate *priv;
} DmapdDPAPRecordFactory;

typedef struct {
	GObjectClass parent;
} DmapdDPAPRecordFactoryClass;

GType                  dmapd_dpap_record_factory_get_type (void);

DMAPRecord            *dmapd_dpap_record_factory_create   (DMAPRecordFactory *factory, gpointer user_data);

#endif /* __DMAPD_DPAP_RECORD_FACTORY */

G_END_DECLS
