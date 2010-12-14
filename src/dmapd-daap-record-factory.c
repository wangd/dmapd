/*
 * DAAPRecord factory class
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

#include "dmapd-daap-record-factory.h"
#include "dmapd-daap-record.h"
#include "av-meta-reader.h"

static DmapdDAAPRecordFactory *factory_singleton;

struct DmapdDAAPRecordFactoryPrivate {
	AVMetaReader *av_meta_reader;
};

enum {
	PROP_0,
	PROP_META_READER
};

static void
dmapd_daap_record_factory_set_property (GObject *object,
					guint prop_id,
					const GValue *value,
					GParamSpec *pspec)
{
	DmapdDAAPRecordFactory *factory = DMAPD_DAAP_RECORD_FACTORY (object);

	switch (prop_id) {
		case PROP_META_READER:
			if (factory->priv->av_meta_reader)
				g_object_unref (factory->priv->av_meta_reader);
			factory->priv->av_meta_reader = AV_META_READER (g_value_get_pointer (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
dmapd_daap_record_factory_get_property (GObject *object,
					guint prop_id,
					GValue *value,
					GParamSpec *pspec)
{
	DmapdDAAPRecordFactory *factory = DMAPD_DAAP_RECORD_FACTORY (object);

	switch (prop_id) {
		case PROP_META_READER:
			g_value_set_pointer (value, factory->priv->av_meta_reader);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

DMAPRecord *
dmapd_daap_record_factory_create  (DMAPRecordFactory *factory,
				   gpointer user_data)
{
	return DMAP_RECORD (dmapd_daap_record_new ((const char *) user_data, DMAPD_DAAP_RECORD_FACTORY (factory)->priv->av_meta_reader));
}

static GObject *dmapd_daap_record_factory_constructor (GType type,
			       guint n_construct_properties,
			       GObjectConstructParam *construct_properties)
{
	GObject *object;

	if (! factory_singleton) {
		DmapdDAAPRecordFactoryClass *klass;
		GObjectClass *parent_class;
		klass = DMAPD_DAAP_RECORD_FACTORY_CLASS (
				g_type_class_peek (TYPE_DMAPD_DAAP_RECORD_FACTORY));
		parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
		object = G_OBJECT_CLASS (parent_class)->constructor (type,
							n_construct_properties,
							construct_properties);
		factory_singleton = DMAPD_DAAP_RECORD_FACTORY (object);
	} else {
		object = g_object_ref (G_OBJECT (factory_singleton));
	}

	return object;
}

static void
dmapd_daap_record_factory_init (DmapdDAAPRecordFactory *factory)
{
	factory->priv = DMAPD_DAAP_RECORD_FACTORY_GET_PRIVATE (factory);
}

static void
dmapd_daap_record_factory_class_init (DmapdDAAPRecordFactoryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DmapdDAAPRecordFactoryPrivate));

        gobject_class->set_property = dmapd_daap_record_factory_set_property;
	gobject_class->get_property = dmapd_daap_record_factory_get_property;
	gobject_class->constructor  = dmapd_daap_record_factory_constructor;

	g_object_class_install_property (gobject_class, PROP_META_READER,
					 g_param_spec_pointer ("meta-reader",
					 		      "AV Meta Reader",
							      "AV Meta Reader",
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
}

static void
dmapd_daap_record_factory_interface_init (gpointer iface, gpointer data)
{
	DMAPRecordFactoryIface *factory = iface;

	g_assert (G_TYPE_FROM_INTERFACE (factory) == DMAP_TYPE_RECORD_FACTORY);

	factory->create = dmapd_daap_record_factory_create;
}

G_DEFINE_TYPE_WITH_CODE (DmapdDAAPRecordFactory, dmapd_daap_record_factory, G_TYPE_OBJECT, 
			 G_IMPLEMENT_INTERFACE (DMAP_TYPE_RECORD_FACTORY,
					        dmapd_daap_record_factory_interface_init))
