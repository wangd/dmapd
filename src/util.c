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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "dmapd-module.h"
#include "dmapd-dmap-db.h"
#include "dmapd-dmap-db-ghashtable.h"
#include "db-builder.h"
#include "av-meta-reader.h"
#include "av-render.h"
#include "photo-meta-reader.h"

static GHashTable *stringleton;

gchar *
parse_plugin_option (gchar *str, GHashTable *hash_table)
{
	// The user may specify, e.g.:
	//   "gst:sink=foo,opt1=bar,opt2=baz"
	// or
	//   "gst"

	gchar *plugin = str;
	gchar *colon = strchr (str, ':');

	if (colon) {
		gchar *eq, *key;
		*colon = 0x00;
		key = colon + 1;	
		eq = strchr (key, '=');
		if (str && eq) {
			gchar *comma;
			do {
				gchar *val;
				*eq = 0x00;
				val = eq + 1;
				comma = strchr (val, ',');
				if (comma) {
					*comma = 0x00;
				}
				g_hash_table_insert (hash_table, g_strdup (key), g_strdup(val));
				if (comma) {
					key = comma + 1;
					eq = strchr (key, '=');
					if (! str || ! eq) {
						g_error ("Badly formatted plugin string");
					}
				}
			} while (comma);
		} else {
			g_error ("Badly formatted plugin string");
		}
	}

	return plugin;
}

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

	g_assert (stringleton);

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

	g_debug ("        Increment stringleton %s reference count to %u.", str, GPOINTER_TO_UINT (val));

	return str;
}

void stringleton_unref (const gchar *str)
{
	guint count;

	g_assert (stringleton);

	if (str != NULL) {
		count = GPOINTER_TO_UINT (g_hash_table_lookup (stringleton,
							      (gpointer) str));

		g_debug ("        Decrement stringleton %s reference count to %u.", str, count - 1);

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
	else if (type == TYPE_AV_RENDER)
		return "av-render-%s";
	else if (type == TYPE_PHOTO_META_READER)
		return "photo-meta-reader-%s";
	else
		return NULL;
}


GObject *
object_from_module (GType type, const gchar *module_dir, const gchar *module_name, const gchar *first_property_name, ...)
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
		g_debug ("Not loading built in %s.", g_type_name (TYPE_DMAPD_DMAP_DB_GHASHTABLE));
		child_type = TYPE_DMAPD_DMAP_DB_GHASHTABLE;
		fnval = g_object_new_valist (child_type, first_property_name, ap);
	} else {
		module_filename = g_strdup_printf (fmt, module_name);
		module_path = g_module_build_path (module_dir, module_filename);

		module = dmapd_module_new (module_path);
		if (module == NULL || ! g_type_module_use (G_TYPE_MODULE (module))) {
			g_warning ("Error opening %s", module_path);
		} else {
			/* FIXME: free filters */
			filters = g_type_children (type, &n_filters);
			g_assert (n_filters == 1);
			g_assert (g_type_is_a (filters[0], type));

			child_type = filters[0];
			fnval = g_object_new_valist (child_type, first_property_name, ap);
		}

		if (filters)
			g_free (filters);

		g_free (module_filename);
		g_free (module_path);
	}

	va_end (ap);

	return fnval;
}

gboolean
dmapd_util_hash_file (const gchar *uri, unsigned char hash[DMAP_HASH_SIZE])
{
	g_assert (NULL != uri);
	g_assert (NULL != hash);

	gsize bytes_read = 0;
	gboolean fnval = FALSE;
	GError *error = NULL;
	GFile *file = NULL;
	GFileInputStream *stream = NULL;
	unsigned char buffer[BUFSIZ];
	DMAPHashContext context;

	file = g_file_new_for_uri (uri);
	if (NULL == file) {
		g_warning ("Could not open %s\n", uri);
		goto _done;
	}

	stream = g_file_read (file, NULL, &error);
	if (error != NULL) {
		g_warning ("Could not read %s: %s\n", uri, error->message);
		goto _done;
	}

	dmap_hash_progressive_init (&context);

	while (0 < (bytes_read = g_input_stream_read (G_INPUT_STREAM (stream), buffer, BUFSIZ, NULL, &error))) {
		dmap_hash_progressive_update (&context, buffer, bytes_read);
	}	

	if (NULL != error) {
		g_warning ("Could not read %s: %s\n", uri, error->message);
		goto _done;
	}

	dmap_hash_progressive_final (&context, hash);

	fnval = TRUE;

_done:
	if (NULL != file) {
		g_object_unref (file);
	}

	if (NULL != stream) {
		g_object_unref (stream);
	}

	return fnval;
}

gchar *
cache_path (cache_type_t type, const gchar *db_dir, const gchar *uri)
{
        gchar *cachepath = NULL;
	guchar raw_hash[DMAP_HASH_SIZE] = { 0 };
        guchar hash[DMAP_HASH_SIZE * 2 + 1] = { 0 };
	
	if (! dmapd_util_hash_file (uri, raw_hash)) {
		goto _done;
	}

	dmap_hash_progressive_to_string (raw_hash, hash);

	switch (type) {
	case CACHE_TYPE_RECORD:
		cachepath = g_strdup_printf ("%s/%s.%s", db_dir, hash, "record");
		break;
	case CACHE_TYPE_TRANSCODED_DATA:
		/* FIXME: set extension properly? */
		cachepath = g_strdup_printf ("%s/%s.%s", db_dir, hash, "data");
		break;
	case CACHE_TYPE_THUMBNAIL_DATA:
		cachepath = g_strdup_printf ("%s/%s.%s", db_dir, hash, "thumb");
		break;
	default:
		g_error ("Bad cache path type");
	}

_done:
        return cachepath;
}

void
cache_store (const gchar *db_dir, const gchar *uri, GByteArray *blob)
{
        struct stat st;
        gchar *cachepath = NULL;
        GError *error = NULL;
        /* NOTE: g_stat seemed broken; would corrupt GError *error. */
        if (stat (db_dir, &st) != 0) {
                g_warning ("cache directory %s does not exist, will not cache", db_dir);
		goto _done;
        }
        if (! (st.st_mode & S_IFDIR)) {
                g_warning ("%s is not a directory, will not cache", db_dir);
		goto _done;
        }
        cachepath = cache_path (CACHE_TYPE_RECORD, db_dir, uri);
	if (NULL == cachepath) {
		goto _done;
	}

        g_file_set_contents (cachepath,
			    (gchar *) blob->data,
			     blob->len,
			     &error);
        if (error != NULL) {
                g_warning ("Error writing %s: %s", cachepath, error->message);
		goto _done;
        }

_done:
	if (NULL != cachepath) {
		g_free (cachepath);
	}

	return;
}
