/*
 * Database class for DMAP sharing
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

#include <glib/ghash.h>

#include "dmapd-dmap-container-db.h"
#include "dmapd-dmap-container-record.h"

struct DmapdDMAPContainerDbPrivate {
	GHashTable *db;
};

DMAPContainerRecord *
dmapd_dmap_container_db_lookup_by_id (DMAPContainerDb *db, guint id)
{
	DMAPContainerRecord *record;
	record = g_hash_table_lookup (DMAPD_DMAP_CONTAINER_DB (db)->priv->db, GUINT_TO_POINTER (id));
	g_object_ref (record);
	return record;
}

void
dmapd_dmap_container_db_foreach (DMAPContainerDb *db,
				 void (*fn) (gpointer key,
					     gpointer value,
					     gpointer user_data),
					     gpointer data)
{
	g_hash_table_foreach (DMAPD_DMAP_CONTAINER_DB (db)->priv->db, (GHFunc) fn, data);
}

gint64
dmapd_dmap_container_db_count (DMAPContainerDb *db)
{
	return g_hash_table_size (DMAPD_DMAP_CONTAINER_DB (db)->priv->db);
}

void
dmapd_dmap_container_db_add (DMAPContainerDb *db, DMAPContainerRecord *record)
{
        guint id = dmap_container_record_get_id (record);
	g_object_ref (record);
	g_hash_table_insert (DMAPD_DMAP_CONTAINER_DB (db)->priv->db, GUINT_TO_POINTER (id), record);
}

static void
dmapd_dmap_container_db_init (DmapdDMAPContainerDb *db)
{
	db->priv = DMAPD_DMAP_CONTAINER_DB_GET_PRIVATE (db);
	db->priv->db = g_hash_table_new_full (g_direct_hash,
					      g_direct_equal,
					      NULL,
					      g_object_unref);
}

static void
dmapd_dmap_container_db_finalize (GObject *object)
{
        DmapdDMAPContainerDb *db = DMAPD_DMAP_CONTAINER_DB (object);

	g_debug ("Finalizing DmapdDMAPContainerDb (%d records)", g_hash_table_size (db->priv->db));

	g_hash_table_destroy (db->priv->db);
}

static void
dmapd_dmap_container_db_class_init (DmapdDMAPContainerDbClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = dmapd_dmap_container_db_finalize;

        g_type_class_add_private (klass, sizeof (DmapdDMAPContainerDbPrivate));
}

static void
dmapd_dmap_container_db_interface_init (gpointer iface, gpointer data)
{
	DMAPContainerDbInterface *dmap_container_db = iface;

	g_assert (G_TYPE_FROM_INTERFACE (dmap_container_db) == TYPE_DMAP_CONTAINER_DB);

	dmap_container_db->lookup_by_id = dmapd_dmap_container_db_lookup_by_id;
	dmap_container_db->foreach = dmapd_dmap_container_db_foreach;
	dmap_container_db->count = dmapd_dmap_container_db_count;
}

G_DEFINE_TYPE_WITH_CODE (DmapdDMAPContainerDb, dmapd_dmap_container_db, G_TYPE_OBJECT, 
			 G_IMPLEMENT_INTERFACE (TYPE_DMAP_CONTAINER_DB,
						dmapd_dmap_container_db_interface_init))

DmapdDMAPContainerDb *
dmapd_dmap_container_db_new (void)
{
	DmapdDMAPContainerDb *db;

	db = DMAPD_DMAP_CONTAINER_DB (g_object_new (TYPE_DMAPD_DMAP_CONTAINER_DB, NULL));

	return db;
}
