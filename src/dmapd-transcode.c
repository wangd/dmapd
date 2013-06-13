/*   FILE: dmapd-transcode.c -- Perform non-real-time transcoding. Even
 *         though dmapd can do this itself, a separate tool might be desireable if
 *         dmapd itself is running on a slow computer.
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 13 June 2013
 *
 * Copyright (c) 2013 W. Michael Petullo <new@flyn.org>
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

#include <config.h>
#include <stdlib.h>
#include <glib.h>

#include "dmapd-dmap-db.h"
#include "dmapd-daap-record.h"
#include "dmapd-daap-record-factory.h"
#include "db-builder.h"
#include "av-meta-reader.h"
#include "util-gst.h"

#define DEFAULT_DB_MOD                "ghashtable"
#define DEFAULT_AV_META_READER_MOD    "gst"

GMainLoop *loop;

static GSList  *music_dirs               = NULL;
static GSList  *music_formats            = NULL;
static gchar   *module_dir               = NULL;
static gchar   *output_dir               = NULL;
static gchar   *transcode_mimetype       = NULL;
static gchar   *db_module                = NULL;
static gchar   *av_meta_reader_module    = NULL;
static gboolean enable_version           = FALSE;

static void
free_globals (void)
{
	g_debug ("Free'ing globals");

	slist_deep_free (music_dirs);
	slist_deep_free (music_formats);

	g_free (output_dir);
}

static gboolean
add_to_opt_list (const gchar *option_name,
	         const gchar *value,
	         gpointer data,
	         GError **error)
{
	if (strcmp (option_name, "-m") == 0) {
		music_dirs = g_slist_append (music_dirs, g_strdup (value));
	} else if (strcmp (option_name, "-M") == 0) {
		music_formats = g_slist_append (music_formats, g_strdup (value));
	} else {
		g_error ("Unsupported option: %s.", option_name);
	}
	return TRUE;
}

/* FIXME: how to enumerate available transcoding formats? */
static GOptionEntry entries[] = {
	{ "music-dir", 'm', 0, G_OPTION_ARG_CALLBACK, add_to_opt_list, "Music directory", NULL },
	{ "music-format", 'M', 0, G_OPTION_ARG_CALLBACK, add_to_opt_list, "Acceptable music format", NULL },
	{ "output-dir", 'o', 0, G_OPTION_ARG_STRING, &output_dir, "Output directory", NULL },
	{ "transcode-mimetype", 't', 0, G_OPTION_ARG_STRING, &transcode_mimetype, "Target MIME type for transcoding", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &enable_version, "Print version number and exit", NULL },
	{ NULL }
};

static void
transcode (DMAPRecordFactory *factory,
           GSList *media_dirs,
           GSList *acceptable_formats)
{
	GSList *l;
	DMAPDb *db                    = NULL;
	DMAPContainerDb *container_db = NULL;
	DbBuilder *builder            = NULL;

	g_assert (db_module);

	db = DMAP_DB (object_from_module (TYPE_DMAPD_DMAP_DB, 
	                                  module_dir,
					  db_module,
					  "record-factory",
					  factory,
					  NULL));
	g_assert (db);

	if (acceptable_formats) {
		g_object_set (db, "acceptable-formats", acceptable_formats, NULL);
	}

	container_db = DMAP_CONTAINER_DB (dmapd_dmap_container_db_new ());
	builder = DB_BUILDER (object_from_module (TYPE_DB_BUILDER, module_dir, "gdir", NULL));

	for (l = media_dirs; l; l = l->next) {
		db_builder_build_db_starting_at (builder, l->data, db, NULL, NULL);
	}

	dmap_db_foreach (db, (GHFunc) transcode_cache, output_dir);

	if (NULL != db) {
		g_object_unref (db);
	}

	if (NULL != container_db) {
		g_object_unref (container_db);
	}

	if (NULL != builder) {
		g_object_unref (builder);
	}
}

static void sigterm_handler (int i)
{
	signal (SIGTERM, sigterm_handler);

	g_debug ("Received TERM signal");

	g_main_loop_quit (loop);
}

static void
debug_printf (const char *log_domain,
	      GLogLevelFlags log_level,
	      const gchar *message,
	      gpointer user_data)
{
	g_print ("%s\n", message);
}

static void
debug_null (const char *log_domain,
	    GLogLevelFlags log_level,
	    const gchar *message,
	    gpointer user_data)
{
}

int main (int argc, char *argv[])
{
	int exitval = EXIT_SUCCESS;
	GError *error = NULL;
	GOptionContext *context;
	AVMetaReader *av_meta_reader = NULL;

	g_type_init ();
	stringleton_init ();

	loop = g_main_loop_new (NULL, FALSE);

	if (getenv ("DMAPD_DEBUG") != NULL) {
		g_log_set_handler ("libdmapsharing", G_LOG_LEVEL_DEBUG, debug_printf, NULL);
		g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_printf, NULL);
	} else {
		g_log_set_handler ("libdmapsharing", G_LOG_LEVEL_DEBUG, debug_null, NULL);
		g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_null, NULL);
	}

	// These must appear before parsing command-line options because
	// they contribute to which options are available.
	module_dir = getenv ("DMAPD_MODULEDIR");
	module_dir = module_dir ? module_dir : DEFAULT_MODULEDIR;
	
	av_meta_reader_module = getenv ("DMAPD_AV_META_READER_MODULE");
	av_meta_reader_module = av_meta_reader_module ? av_meta_reader_module : DEFAULT_AV_META_READER_MOD;

	db_module = getenv ("DMAPD_DB_MODULE");
	db_module = db_module ? db_module : DEFAULT_DB_MOD;

	context = g_option_context_new ("-m: transcode media");
	g_option_context_add_main_entries (context, entries, NULL);

	if (strcmp (av_meta_reader_module, "null") != 0) {
		GOptionGroup *group;
		av_meta_reader = AV_META_READER (object_from_module (TYPE_AV_META_READER,
		                                                     module_dir,
								     av_meta_reader_module,
								     NULL));
		if (av_meta_reader) {
			group = av_meta_reader_get_option_group (av_meta_reader);
			if (group)
				g_option_context_add_group (context, group);
		}
	}

	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_error ("Option parsing failed: %s", error->message);
	}

	if (enable_version) {
		g_print ("dmapd version %s\n", VERSION);
		exit (EXIT_SUCCESS);
	}

	if (! av_meta_reader) {
		g_error ("Need an AV metadata reader plugin");
	}

	if (! music_dirs) {
		g_error ("Need to provide a music directory");
	}

	if (! output_dir) {
		output_dir = g_strdup (".");
	}

	g_option_context_free (context);

	signal (SIGTERM, sigterm_handler);

	if (av_meta_reader == NULL)
		g_error ("Music directory specified but AV metadata reader module is 'null'");
	DMAPRecordFactory *factory;
	factory = DMAP_RECORD_FACTORY (
			g_object_new (
				TYPE_DMAPD_DAAP_RECORD_FACTORY,
				"meta-reader",
				av_meta_reader,
				NULL));

	transcode (factory, music_dirs, music_formats);

	if (av_meta_reader)
		g_object_unref (av_meta_reader);

	free_globals ();
	stringleton_deinit ();
	g_debug ("Parent Exiting");

	exit(exitval);
}
