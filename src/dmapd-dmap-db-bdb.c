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

#include <db.h>
#include <stdlib.h>
#include <string.h>
#include <libdmapsharing/dmap.h>

#include "util.h"
#include "dmapd-dmap-db-bdb.h"

const gchar *DB_FILENAME = "dmapd.db";

/* Media ID's start at max and go down. Container ID's start at 1 and go up. */
static guint nextid = G_MAXINT; /* NOTE: this should be G_MAXUINT, but iPhoto can't handle it. */

struct DmapdDMAPDbBDBPrivate {
	DB_ENV *env;
	DB *db;
};

static DMAPRecord *
dmapd_dmap_db_bdb_lookup_by_id	(const DMAPDb *db, guint id)
{
	DBT key, data;
	GByteArray *blob;
	DMAPRecord *record = NULL;
	DMAPRecordFactory *factory = NULL;
	DmapdDMAPDbBDBPrivate *priv = DMAPD_DMAP_DB_BDB (db)->priv;

	if (! priv->db)
		goto _return;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof (id);

	if (priv->db->get (priv->db, NULL, &key, &data, 0) != 0) {
		g_warning ("Error finding ID %u in Berkeley Database", id);
		goto _return;
	}

	g_object_get (DMAPD_DMAP_DB (db), "record-factory", &factory, NULL);
	g_assert (factory);
	record = dmap_record_factory_create (factory, NULL);

	blob = g_byte_array_sized_new (data.size);
	g_byte_array_append (blob, data.data, data.size);
	dmap_record_set_from_blob (DMAP_RECORD (record), blob);

	g_byte_array_unref (blob);

_return:
	return record;
}

static guint
dmapd_dmap_db_bdb_lookup_id_by_location (const DMAPDb *db, const gchar *location)
{
	int id;
	guint fnval = 0;
	gboolean found = FALSE;
	/* FIXME: this code is copied elsewhere in this class: */
	/* id == 0 indicates rollover! */
	// NOTE: this should be = G_MAXUINT, not G_MAXINT, but iPhoto can't handle full range of guint:
	for (id = G_MAXINT; id > nextid && ! found; id--) {
		const gchar *_location = NULL;
		DMAPRecord *record = dmapd_dmap_db_bdb_lookup_by_id (db, id);
		if (record) {
			g_object_get (record, "location", &_location, NULL);
			g_assert (_location);
			g_object_unref (record);
			if (! strcmp (_location, location)) {
				fnval = id;
				found = TRUE;
			}
			g_free (_location);
		}
	}

	return fnval;
}

static void
dmapd_dmap_db_bdb_foreach	(const DMAPDb *db,
				 GHFunc func,
				 gpointer data)
{
	guint id;
	/* id == 0 indicates rollover! */
	// NOTE: this should be = G_MAXUINT, not G_MAXINT, but iPhoto can't handle full range of guint:
	for (id = G_MAXINT; id > nextid; id--) {
		DMAPRecord *record = dmapd_dmap_db_bdb_lookup_by_id (db, id);
		func (GUINT_TO_POINTER (id), record, data);
		g_object_unref (record);
	}
}

static gint64
dmapd_dmap_db_bdb_count (const DMAPDb *db)
{
	gint64 fnval;
	DB_BTREE_STAT *statp;
	DmapdDMAPDbBDBPrivate *priv = DMAPD_DMAP_DB_BDB (db)->priv;

	priv->db->stat (priv->db, NULL, &statp, 0);
	fnval = statp->bt_ndata;
	free (statp);

	return fnval;
}

static guint
dmapd_dmap_db_bdb_add_with_id (DMAPDb *_db, DMAPRecord *record, guint id)
{
	int ret;
	DBT key, data;
	DmapdDMAPDbBDB *db = DMAPD_DMAP_DB_BDB (_db);
	DmapdDMAPDbBDBPrivate *priv = DMAPD_DMAP_DB_BDB (db)->priv;

	g_debug ("Adding record with ID %u", id);

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof (id);

	GByteArray *blob = dmap_record_to_blob (record);
	data.data = blob->data;
	data.size = blob->len;

	if ((ret = priv->db->put (priv->db, NULL, &key, &data, 0)) != 0) {
		priv->env->err (priv->env, ret, NULL);
		g_error ("Error inserting into Berkeley Database");
	}

	g_byte_array_unref (blob);

	return id;
}

static guint
dmapd_dmap_db_bdb_add (DMAPDb *db, DMAPRecord *record)
{
	return dmapd_dmap_db_bdb_add_with_id (db, record, nextid--);
}

static guint
dmapd_dmap_db_bdb_add_path (DMAPDb *db, const gchar *path)
{
	guint id = 0;
	DMAPRecord *record;
	DMAPRecordFactory *factory = NULL;

	g_object_get (db, "record-factory", &factory, NULL);
	g_assert (factory);
	record = dmap_record_factory_create (factory, (gpointer) path);

	if (record) {
		id = dmapd_dmap_db_bdb_add (db, record);
		g_object_unref (record);
	} else {
		id = 0;
	}

	return id;
}

G_DEFINE_DYNAMIC_TYPE (DmapdDMAPDbBDB,
		       dmapd_dmap_db_bdb,
		       TYPE_DMAPD_DMAP_DB)

static GObject*
dmapd_dmap_db_bdb_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
        DmapdDMAPDbBDB *db;
	gchar *db_dir = NULL;

	db = DMAPD_DMAP_DB_BDB(G_OBJECT_CLASS (dmapd_dmap_db_bdb_parent_class)->constructor (type, n_construct_params, construct_params));

	gint64 numrec;
	gchar *db_path = NULL;

	db->priv = DMAPD_DMAP_DB_BDB_GET_PRIVATE (db);

	g_object_get (db, "db-dir", &db_dir, NULL);
	g_assert (db_dir);
	db->priv->env->set_data_dir  (db->priv->env, db_dir);
	db->priv->env->set_cachesize (db->priv->env, 0, 1 * 1024 * 1024, 1);
	db->priv->env->set_cache_max (db->priv->env, 0, 1 * 1024 * 1024);

	if (db->priv->env->open (db->priv->env, db_dir, DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL, 0) != 0)
		g_error ("Could not open Berkeley Database environment");

	if (db_create(&(db->priv->db), db->priv->env, 0) != 0)
		g_error ("Could not initialize Berkeley Database");

	db_path = g_strdup_printf ("%s/%s", db_dir, DB_FILENAME);
	if (db->priv->db->open (db->priv->db, NULL, db_path, NULL,
	    DB_BTREE, DB_CREATE, 0) != 0) {
		g_free (db_path);
		g_error ("Could not open Berkeley Database");
	}

	numrec = dmapd_dmap_db_bdb_count (DMAP_DB (db));
	g_debug ("Opened database with %" G_GINT64_FORMAT " records", numrec);
	// NOTE: this should be G_MAXUINT, not G_MAXINT, but iPhoto can't handle full range of guint:
	nextid = G_MAXINT - numrec;

	g_free (db_dir);

	return G_OBJECT (db);
}


static void dmapd_dmap_db_bdb_init (DmapdDMAPDbBDB *db)
{
	db->priv = DMAPD_DMAP_DB_BDB_GET_PRIVATE (db);

	if (db_env_create (&(db->priv->env), 0) != 0)
		g_error ("Could not create Berkeley Database environment");
}

static void
dmapd_dmap_db_bdb_class_finalize (DmapdDMAPDbBDBClass *object)
{
}

static void
dmapd_dmap_db_bdb_finalize (GObject *object)
{
        DB_BTREE_STAT *statp;
	DmapdDMAPDbBDBPrivate *priv = DMAPD_DMAP_DB_BDB (object)->priv;

	priv->db->stat (priv->db, NULL, &statp, 0);
	g_debug ("Finalizing DmapdDMAPDbBDB (%d records)", statp->bt_ndata);
	free (statp);
	priv->db->close (priv->db, 0);
	priv->env->close (priv->env, 0);
}

static void dmapd_dmap_db_bdb_class_init (DmapdDMAPDbBDBClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	DmapdDMAPDbClass *dmap_db_class = DMAPD_DMAP_DB_CLASS (klass);

	object_class->finalize = dmapd_dmap_db_bdb_finalize;
	object_class->constructor = dmapd_dmap_db_bdb_constructor;

	dmap_db_class->add = dmapd_dmap_db_bdb_add;
	dmap_db_class->add_with_id = dmapd_dmap_db_bdb_add_with_id;
	dmap_db_class->add_path = dmapd_dmap_db_bdb_add_path;
	dmap_db_class->lookup_by_id = dmapd_dmap_db_bdb_lookup_by_id;
	dmap_db_class->lookup_id_by_location = dmapd_dmap_db_bdb_lookup_id_by_location;
	dmap_db_class->foreach = dmapd_dmap_db_bdb_foreach;
	dmap_db_class->count = dmapd_dmap_db_bdb_count;

	g_type_class_add_private (klass, sizeof (DmapdDMAPDbBDBPrivate));
}

static void dmapd_dmap_db_bdb_register_type (GTypeModule *module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule *module)
{
	dmapd_dmap_db_bdb_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT gboolean
dmapd_module_unload (GTypeModule *module)
{
	return TRUE;
}
