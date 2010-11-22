/*
 *  Database class for DMAP sharing
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

#include <libdmapsharing/dmap.h>

#include "dmapd-dmap-db.h"

struct DmapdDMAPDbPrivate {
	gchar *db_dir;
	DMAPRecordFactory *record_factory;
};

enum {
	PROP_0,
	PROP_DB_DIR,
	PROP_RECORD_FACTORY
};

static void dmapd_dmap_db_init (DmapdDMAPDb *db)
{
	db->priv = DMAPD_DMAP_DB_GET_PRIVATE (db);
}

static void
dmapd_dmap_db_set_property (GObject *object,
                            guint prop_id,
			    const GValue *value,
			    GParamSpec *pspec)
{
        DmapdDMAPDb *db = DMAPD_DMAP_DB (object);

	switch (prop_id) {
		case PROP_DB_DIR:
			g_free (db->priv->db_dir);
			db->priv->db_dir = g_value_dup_string (value);
			break;
		case PROP_RECORD_FACTORY:
			if (db->priv->record_factory)
				g_object_unref (db->priv->record_factory);
			db->priv->record_factory = DMAP_RECORD_FACTORY (g_value_get_pointer (value));
			break;	
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
dmapd_dmap_db_get_property (GObject *object,
                            guint prop_id,
			    GValue *value,
			    GParamSpec *pspec)
{
        DmapdDMAPDb *db = DMAPD_DMAP_DB (object);

	switch (prop_id) {
		case PROP_DB_DIR:
			g_value_set_static_string (value, db->priv->db_dir);
			break;
		case PROP_RECORD_FACTORY:
			g_value_set_pointer (value, db->priv->record_factory);
			break;	
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void dmapd_dmap_db_class_init (DmapdDMAPDbClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DmapdDMAPDbPrivate));

	gobject_class->set_property = dmapd_dmap_db_set_property;
	gobject_class->get_property = dmapd_dmap_db_get_property;
	/* FIXME: implement finalize */

	g_object_class_install_property (gobject_class, PROP_RECORD_FACTORY,
					 g_param_spec_pointer ("record-factory",
							       "Record factory",
							       "Record factory",
					 		        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (gobject_class, PROP_DB_DIR,
					 g_param_spec_string ("db-dir",
							      "Directory for database cache",
							      "Directory for database cache",
				 	 		       NULL,
					 		       G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	}

static guint add (DMAPDb *db, DMAPRecord *record)
{
	return DMAPD_DMAP_DB_GET_CLASS (db)->add (db, record);
}

static guint add_with_id (DMAPDb *db, DMAPRecord *record, guint id)
{
	return DMAPD_DMAP_DB_GET_CLASS (db)->add_with_id (db, record, id);
}

static guint add_path (DMAPDb *db, const gchar *path)
{
	return DMAPD_DMAP_DB_GET_CLASS (db)->add_path (db, path);
}

DMAPRecord *lookup_by_id (const DMAPDb *db, guint id)
{
	return DMAPD_DMAP_DB_GET_CLASS (db)->lookup_by_id (db, id);
}

static guint lookup_id_by_location (const DMAPDb *db, const gchar *location)
{
	return DMAPD_DMAP_DB_GET_CLASS (db)->lookup_id_by_location (db, location);
}

static void foreach (const DMAPDb *db, GHFunc func, gpointer data)
{
	return DMAPD_DMAP_DB_GET_CLASS (db)->foreach (db, func, data);
}

static gint64 count (const DMAPDb *db)
{
	return DMAPD_DMAP_DB_GET_CLASS (db)->count (db);
}

static void dmapd_dmap_db_interface_init (gpointer iface, gpointer data)
{
	DMAPDbInterface *dmap_db = iface;

	g_assert (G_TYPE_FROM_INTERFACE (dmap_db) == TYPE_DMAP_DB);

	dmap_db->add = add;
	dmap_db->add_with_id = add_with_id;
	dmap_db->add_path = add_path;
	dmap_db->lookup_by_id = lookup_by_id;
	dmap_db->lookup_id_by_location = lookup_id_by_location;
	dmap_db->foreach = foreach;
	dmap_db->count = count;
}

G_DEFINE_TYPE_WITH_CODE (DmapdDMAPDb, dmapd_dmap_db, G_TYPE_OBJECT, 
			 G_IMPLEMENT_INTERFACE (TYPE_DMAP_DB, dmapd_dmap_db_interface_init))
