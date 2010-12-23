/*   FILE: util.c -- utility functions
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 01 January 2009 
 *
 * Copyright (c) 2009 W. Michael Petullo <new@flyn.org>
 * All rights reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libdmapsharing/dmap.h>

#include "util.h"
#include "dmapd-module.h"
#include "dmapd-dmap-db.h"
#include "dmapd-dmap-db-ghashtable.h"
#include "db-builder.h"
#include "av-meta-reader.h"
#include "photo-meta-reader.h"

static GHashTable *stringleton;

GByteArray *
blob_add_atomic (GByteArray *blob, const guint8 *ptr, const size_t size)
{
	return g_byte_array_append (blob, ptr, size);
}

GByteArray *
blob_add_string (GByteArray *blob, const gchar *str)
{
	return g_byte_array_append (blob,
				   (const guint8 *) str,
				    strlen (str) + 1);
}

void
slist_deep_free (GSList *list)
{
	g_slist_foreach (list, (GFunc)g_free, NULL);
	g_slist_free (list);
}

void stringleton_init (void)
{
	static gboolean initialized = FALSE;

	if (! initialized) {
		stringleton = g_hash_table_new_full (g_str_hash,
						     g_str_equal,
						     g_free,
						     NULL);
	}

	initialized = TRUE;
}

const gchar *stringleton_ref (const gchar *str)
{
	gpointer key;
	gpointer val;

	/* NOTE: insert will free passed str if the key already exists,
	 * not existing key in hash table.
	 */
	if (g_hash_table_lookup_extended (stringleton, str, &key, &val)) {
		str = (gchar *) key;
		g_hash_table_insert (stringleton,
				    (gpointer) g_strdup (str),
		                     val + 1);
	} else {
		val = NULL;
		str = g_strdup (str);
		g_hash_table_insert (stringleton,
				    (gpointer) str,
		                     val + 1);
	}

	g_debug ("stringleton ref %s to %u", str, GPOINTER_TO_UINT (val));

	return str;
}

void stringleton_unref (const gchar *str)
{
	guint count;

	if (str != NULL) {
		count = GPOINTER_TO_UINT (g_hash_table_lookup (stringleton,
							      (gpointer) str));

		g_debug ("stringleton unref %s to %u", str, count - 1);

		/* NOTE: insert will free passed str if the key already exists,
		 * not existing key in hash table.
		 */
		if (count > 1) {
			g_hash_table_insert (stringleton,
					    (gpointer) g_strdup (str),
					     GUINT_TO_POINTER (count - 1));
		} else if (count == 1) {
			g_hash_table_remove (stringleton, (gpointer) str);
		}
	}
}

void stringleton_deinit (void)
{
	g_hash_table_destroy (stringleton);
}

static char *
find_plugin_template (GType type)
{
	if (type == TYPE_DMAPD_DMAP_DB)
		return "dmapd-dmap-db-%s";
	else if (type == TYPE_DB_BUILDER)
		return "db-builder-%s";
	else if (type == TYPE_AV_META_READER)
		return "av-meta-reader-%s";
	else if (type == TYPE_PHOTO_META_READER)
		return "photo-meta-reader-%s";
	else
		return NULL;
}


GObject *
object_from_module (GType type, const gchar *module_name, const gchar *first_property_name, ...)
{
	va_list ap;
	GType *filters = NULL;
	GType child_type = G_TYPE_INVALID;
	guint n_filters;
	const gchar *fmt;
	gchar *module_filename;
        gchar *module_path;
	GObject *fnval = NULL;
	DmapdModule *module;

	va_start (ap, first_property_name);

	if (! (fmt = find_plugin_template (type))) {
		g_error ("Could not find plugin template");
	}

	/* dmapd-dmap-db-ghashtable is built in because it is used by DmapdDMAPContainerRecord: */
	if (! strcmp (module_name, "ghashtable")) {
		g_debug ("Not loading built in DmapdDMAPDbGHashTable %s", g_type_name (TYPE_DMAPD_DMAP_DB_GHASHTABLE));
		child_type = TYPE_DMAPD_DMAP_DB_GHASHTABLE;
	} else {
		module_filename = g_strdup_printf (fmt, module_name);
		module_path = g_module_build_path (MODULEDIR, module_filename);

		module = dmapd_module_new (module_path);
		if (module == NULL || ! g_type_module_use (G_TYPE_MODULE (module))) {
			g_warning ("Error opening %s", module_path);
		} else {
			/* FIXME: free filters */
			filters = g_type_children (type, &n_filters);
			g_assert (n_filters == 1);
			g_assert (g_type_is_a (filters[0], type));
			child_type = filters[0];
		}

		if (filters)
			g_free (filters);

		g_free (module_filename);
		g_free (module_path);
	}

	fnval = g_object_new_valist (child_type, first_property_name, ap);

	va_end (ap);

	return fnval;
}

gchar *
cache_path (cache_type_t type, const gchar *db_dir, const gchar *imagepath)
{
        gchar *cachepath = NULL;
        guchar hash[33] = { 0 };
	
	/* FIXME: this should really be based on the contents of the file,
	 * not the filename.
	 */
	gchar *filename = strrchr (imagepath, '/') + 1;
	g_assert (filename);

        dmap_hash_generate (1, (const guchar*) filename, 2, hash, 0);

	if (type == CACHE_TYPE_RECORD) {
		cachepath = g_strdup_printf ("%s/%s.%s", db_dir, hash, "record");
	} else if (type == CACHE_TYPE_TRANSCODED_DATA) {
		cachepath = g_strdup_printf ("%s/%s.%s", db_dir, hash, "mp3");
	}

        return cachepath;
}
