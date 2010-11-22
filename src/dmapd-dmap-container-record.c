/*
 * Database record class for DMAP sharing
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

/* FIXME: this uses the GHashTable database even when DmapdDMAPDb uses the
 * BDB.
 */

#include <libdmapsharing/dmap.h>

#include "dmapd-dmap-container-record.h"
#include "dmapd-dmap-db-ghashtable.h"

enum {
	PROP_0,
	PROP_NAME,
	PROP_FULL_DB
};

/* Media ID's start at max and go down.
 * Container ID's start at 1 and go up.
 * 1 is reserved for the primary container.
 */
static gint nextid = 2;

struct DmapdDMAPContainerRecordPrivate {
	gint id;
	char *name;
	GSList *entries;
	DMAPDb *full_db;
};

struct DBPair {
	DMAPDb *src;
	DMAPDb *dest;
};

static void
dmapd_dmap_container_record_set_property (GObject *object,
					  guint prop_id,
					  const GValue *value,
					  GParamSpec *pspec)
{
	DmapdDMAPContainerRecord *record = DMAPD_DMAP_CONTAINER_RECORD (object);

	switch (prop_id) {
		case PROP_NAME:
			g_free (record->priv->name);
			record->priv->name = g_value_dup_string (value);
			break;
		case PROP_FULL_DB:
			if (record->priv->full_db)
				g_object_unref (record->priv->full_db);
			record->priv->full_db = DMAP_DB (g_value_get_pointer (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
dmapd_dmap_container_record_get_property (GObject *object,
					  guint prop_id,
					  GValue *value,
					  GParamSpec *pspec)
{
	DmapdDMAPContainerRecord *record = DMAPD_DMAP_CONTAINER_RECORD (object);

	switch (prop_id) {
		case PROP_NAME:
			g_value_set_static_string (value, record->priv->name);
			break;
		case PROP_FULL_DB:
			g_value_set_pointer (value, record->priv->full_db);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static guint
dmapd_dmap_container_record_get_id (DMAPContainerRecord *record)
{
	return DMAPD_DMAP_CONTAINER_RECORD (record)->priv->id;
}

static void
dmapd_dmap_container_record_add_entry (DMAPContainerRecord *container_record,
				       DMAPRecord *record, gint id)
{
	DmapdDMAPContainerRecordPrivate *priv = 
		DMAPD_DMAP_CONTAINER_RECORD (container_record)->priv;
	priv->entries = g_slist_append (priv->entries, GUINT_TO_POINTER (id));
}

static guint64
dmapd_dmap_container_record_get_entry_count (DMAPContainerRecord *record)
{
	return g_slist_length
		(DMAPD_DMAP_CONTAINER_RECORD (record)->priv->entries);
}

static void
add_to_db (GSList *item, struct DBPair *pair)
{
	DMAPRecord *record = dmap_db_lookup_by_id (pair->src,
						   GPOINTER_TO_UINT (item));
	g_debug ("Adding record %u to container DB", GPOINTER_TO_UINT (item));
	dmap_db_add_with_id (pair->dest,
			     record,
			     GPOINTER_TO_UINT (item));
}

static DMAPDb *
dmapd_dmap_container_record_get_entries (DMAPContainerRecord *record)
{
	struct DBPair pair;

	pair.src  = DMAPD_DMAP_CONTAINER_RECORD (record)->priv->full_db;
	pair.dest = DMAP_DB (g_object_new (TYPE_DMAPD_DMAP_DB_GHASHTABLE, NULL));

	g_assert (DMAPD_DMAP_CONTAINER_RECORD (record)->priv->entries != NULL);

	g_slist_foreach (DMAPD_DMAP_CONTAINER_RECORD (record)->priv->entries,
			 (GFunc) add_to_db,
			 &pair);
	
	return pair.dest;
}

static void
dmapd_dmap_container_record_init (DmapdDMAPContainerRecord *record)
{
	record->priv = DMAPD_DMAP_CONTAINER_RECORD_GET_PRIVATE (record);
	record->priv->id = nextid++;
	record->priv->entries = NULL;
	record->priv->full_db = NULL;
}

static void
dmapd_dmap_container_record_finalize (GObject *object)
{
        DmapdDMAPContainerRecord *db = DMAPD_DMAP_CONTAINER_RECORD (object);

	g_debug ("Finalizing DmapdDMAPContainerRecord (%u records)", g_slist_length (db->priv->entries));

	g_free (db->priv->name);

	g_slist_free (db->priv->entries);

	// G_OBJECT_CLASS (dmapd_dmap_container_record_parent_class)->finalize (object);
}

static void
dmapd_dmap_container_record_class_init (DmapdDMAPContainerRecordClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = dmapd_dmap_container_record_finalize;

        g_type_class_add_private (klass, sizeof (DmapdDMAPContainerRecordPrivate));

	gobject_class->set_property = dmapd_dmap_container_record_set_property;
	gobject_class->get_property = dmapd_dmap_container_record_get_property;

	g_object_class_override_property (gobject_class, PROP_NAME, "name");

	g_object_class_install_property  (gobject_class, PROP_FULL_DB,
					  g_param_spec_pointer ("full-db",
					  			"Full Media DB",
					  			"Full Media DB",
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
dmapd_dmap_container_record_interface_init (gpointer iface, gpointer data)
{
	DMAPContainerRecordInterface *dmap_container_record = iface;

	g_assert (G_TYPE_FROM_INTERFACE (dmap_container_record) == TYPE_DMAP_CONTAINER_RECORD);

	dmap_container_record->get_id = dmapd_dmap_container_record_get_id;
	dmap_container_record->add_entry = dmapd_dmap_container_record_add_entry;
	dmap_container_record->get_entry_count = dmapd_dmap_container_record_get_entry_count;
	dmap_container_record->get_entries = dmapd_dmap_container_record_get_entries;
}

G_DEFINE_TYPE_WITH_CODE (DmapdDMAPContainerRecord, dmapd_dmap_container_record, G_TYPE_OBJECT, 
			G_IMPLEMENT_INTERFACE (TYPE_DMAP_CONTAINER_RECORD,
					       dmapd_dmap_container_record_interface_init))
