/*
 * DPAPRecord factory class
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

#include "util.h"
#include "photo-meta-reader.h"
#include "dmapd-dpap-record.h"
#include "dmapd-dpap-record-factory.h"

static DmapdDPAPRecordFactory *factory_singleton;

struct DmapdDPAPRecordFactoryPrivate {
        PhotoMetaReader *photo_meta_reader;
};

enum {
        PROP_0,
	PROP_META_READER
};

static void
dmapd_dpap_record_factory_set_property (GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
        DmapdDPAPRecordFactory *factory = DMAPD_DPAP_RECORD_FACTORY (object);

        switch (prop_id) {
                case PROP_META_READER:
			if (factory->priv->photo_meta_reader)
				g_object_unref (factory->priv->photo_meta_reader);
                        factory->priv->photo_meta_reader = PHOTO_META_READER (g_value_get_pointer (value));
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dmapd_dpap_record_factory_get_property (GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
        DmapdDPAPRecordFactory *factory = DMAPD_DPAP_RECORD_FACTORY (object);

        switch (prop_id) {
                case PROP_META_READER:
                        g_value_set_pointer (value, factory->priv->photo_meta_reader);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

DMAPRecord *
dmapd_dpap_record_factory_create  (DMAPRecordFactory *factory, gpointer user_data)
{
	return DMAP_RECORD (dmapd_dpap_record_new ((const char *) user_data, DMAPD_DPAP_RECORD_FACTORY (factory)->priv->photo_meta_reader));
}

static GObject *dmapd_dpap_record_factory_constructor (GType type,
                               guint n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        GObject *object;

        if (! factory_singleton) {
                DmapdDPAPRecordFactoryClass *klass;
                GObjectClass *parent_class;
                klass = DMAPD_DPAP_RECORD_FACTORY_CLASS (
                                g_type_class_peek (TYPE_DMAPD_DPAP_RECORD_FACTORY));
                parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
                object = G_OBJECT_CLASS (parent_class)->constructor (type,
                                                        n_construct_properties,
                                                        construct_properties);
                factory_singleton = DMAPD_DPAP_RECORD_FACTORY (object);
        } else {
                object = g_object_ref (G_OBJECT (factory_singleton));
        }

        return object;
}

static void
dmapd_dpap_record_factory_init (DmapdDPAPRecordFactory *factory)
{
	factory->priv = DMAPD_DPAP_RECORD_FACTORY_GET_PRIVATE (factory);
}

static void
dmapd_dpap_record_factory_class_init (DmapdDPAPRecordFactoryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (DmapdDPAPRecordFactoryPrivate));

        gobject_class->set_property = dmapd_dpap_record_factory_set_property;
        gobject_class->get_property = dmapd_dpap_record_factory_get_property;
	gobject_class->constructor  = dmapd_dpap_record_factory_constructor;

        g_object_class_install_property (gobject_class, PROP_META_READER,
                                         g_param_spec_pointer ("meta-reader",
                                                              "Meta Reader",
                                                              "Meta Reader",
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY));
}

static void
dmapd_dpap_record_factory_interface_init (gpointer iface, gpointer data)
{
	DMAPRecordFactoryIface *factory = iface;

	g_assert (G_TYPE_FROM_INTERFACE (factory) == DMAP_TYPE_RECORD_FACTORY);

	factory->create = dmapd_dpap_record_factory_create;
}

G_DEFINE_TYPE_WITH_CODE (DmapdDPAPRecordFactory, dmapd_dpap_record_factory, G_TYPE_OBJECT, 
			 G_IMPLEMENT_INTERFACE (DMAP_TYPE_RECORD_FACTORY,
					        dmapd_dpap_record_factory_interface_init))
