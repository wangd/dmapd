/*   FILE: dmapd.c -- A DMAP server
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

#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <glib.h>
#include <libdmapsharing/dmap.h>

#include "dmapd-dmap-container-record.h"
#include "dmapd-dmap-container-db.h"
#include "dmapd-dpap-record.h"
#include "dmapd-dpap-record-factory.h"
#include "dmapd-daap-record.h"
#include "dmapd-daap-record-factory.h"
#include "dmapd-module.h"
#include "db-builder.h"
#include "av-meta-reader.h"
#include "av-render.h"
#include "photo-meta-reader.h"
#include "util.h"
#include "util-gst.h"

#define DEFAULT_CONFIG_FILE            DEFAULT_SYSCONFDIR "/dmapd.conf"
#define DEFAULT_DB_MOD                "ghashtable"
#define DEFAULT_AV_META_READER_MOD    "gst"
#define DEFAULT_AV_RENDER_MOD         "gst"
#define DEFAULT_PHOTO_META_READER_MOD "vips"

/* For use when deciding whether to serve DAAP or DPAP. */
typedef enum {
	DAAP,
	DPAP,
	DACP
} protocol_id_t;

static char *protocol_map[] = {
	"DAAP",
	"DPAP",
	"DACP",
	NULL,
};

/* Consolidate these so they may be passed to callback, etc. */
typedef struct workers_t {
	DAAPShare *daap_share;
	DPAPShare *dpap_share;
	DACPShare *dacp_share;
	AVRender  *av_render;
} workers_t;

GMainLoop *loop = NULL;

static GSList  *music_dirs               = NULL;
static GSList  *picture_dirs             = NULL;
static GSList  *music_formats            = NULL;
static GSList  *picture_formats          = NULL;
static gchar   *module_dir               = NULL;
static gchar   *config_file              = DEFAULT_CONFIG_FILE;
static gchar   *db_dir                   = DEFAULT_DBDIR;
static gchar   *lockpath                 = DEFAULT_LOCKPATH;
static gchar   *music_password           = NULL;
static gchar   *picture_password         = NULL;
static gchar   *pidpath                  = DEFAULT_RUNDIR "/dmapd.pid";
static gchar   *user                     = NULL;
static gchar   *group                    = NULL;
static gchar   *share_name               = NULL;
static gchar   *transcode_mimetype       = NULL;
static gchar   *db_module                = NULL;
static gchar   *av_meta_reader_module    = NULL;
static gchar   *av_render_module         = NULL;
static gchar   *photo_meta_reader_module = NULL;
static guint    max_thumbnail_width      = 128;
static gboolean enable_dir_containers    = FALSE;
static gboolean enable_foreground        = FALSE;
static gboolean enable_render            = FALSE;
static gboolean enable_rt_transcode      = FALSE;
static gboolean enable_version           = FALSE;
static gboolean exit_after_loading       = FALSE;

// FIXME: make non-global, support mult. remotes and free when done.
// store persistently or set in config file?
static gchar *_guid = NULL;

static void
free_globals (void)
{
	g_debug ("Free'ing globals");

	slist_deep_free (music_dirs);
	slist_deep_free (picture_dirs);
	slist_deep_free (music_formats);
	slist_deep_free (picture_formats);
}

static gboolean
add_to_opt_list (const gchar *option_name,
	         const gchar *value,
	         gpointer data,
	         GError **error)
{
	if (strcmp (option_name, "-m") == 0) {
		music_dirs = g_slist_append (music_dirs, g_strdup (value));
	} else if (strcmp (option_name, "-p") == 0) {
		picture_dirs = g_slist_append (picture_dirs, g_strdup (value));
	} else if (strcmp (option_name, "-M") == 0) {
		music_formats = g_slist_append (music_formats, g_strdup (value));
	} else if (strcmp (option_name, "-P") == 0) {
		picture_formats = g_slist_append (picture_formats, g_strdup (value));
	} else {
		g_error ("Unsupported option: %s.", option_name);
	}
	return TRUE;
}

/* FIXME: how to enumerate available transcoding formats? */
static GOptionEntry entries[] = {
	{ "foreground", 'f', 0, G_OPTION_ARG_NONE, &enable_foreground, "Do not fork; remain in foreground", NULL },
	{ "name", 'n', 0, G_OPTION_ARG_STRING, &share_name, "Name of media shares", NULL },
	{ "music-dir", 'm', 0, G_OPTION_ARG_CALLBACK, add_to_opt_list, "Music directory", NULL },
	{ "picture-dir", 'p', 0, G_OPTION_ARG_CALLBACK, add_to_opt_list, "Picture directory", NULL },
	{ "music-format", 'M', 0, G_OPTION_ARG_CALLBACK, add_to_opt_list, "Acceptable music format", NULL },
	{ "picture-format", 'P', 0, G_OPTION_ARG_CALLBACK, add_to_opt_list, "Acceptable picture format", NULL },
	{ "lockpath", 'l', 0, G_OPTION_ARG_STRING, &lockpath, "Path to lockfile", NULL },
	{ "pidpath", 'i', 0, G_OPTION_ARG_STRING, &pidpath, "Path to PID file", NULL },
	{ "db-dir", 'd', 0, G_OPTION_ARG_STRING, &db_dir, "Media database directory", NULL },
	{ "user", 'u', 0, G_OPTION_ARG_STRING, &user, "User to run as", NULL },
	{ "group", 'g', 0, G_OPTION_ARG_STRING, &group, "Group to run as", NULL },
	{ "render", 'o', 0, G_OPTION_ARG_NONE, &enable_render, "Render using AirPlay", NULL },
	{ "transcode-mimetype", 't', 0, G_OPTION_ARG_STRING, &transcode_mimetype, "Target MIME type for transcoding", NULL },
	{ "rt-transcode", 'r', 0, G_OPTION_ARG_NONE, &enable_rt_transcode, "Perform transcoding in real-time", NULL },
	{ "max-thumbnail-width", 'w', 0, G_OPTION_ARG_INT, &max_thumbnail_width, "Maximum thumbnail size (may reduce memory use)", NULL },
	{ "directory-containers", 'c', 0, G_OPTION_ARG_NONE, &enable_dir_containers, "Serve DMAP containers based on filesystem heirarchy", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &enable_version, "Print version number and exit", NULL },
	{ "exit-after-loading", 'x', 0, G_OPTION_ARG_NONE, &exit_after_loading, "Exit after loading database (do not serve)", NULL },
	{ NULL }
};

static char *
default_share_name (char *media_type)
{
        const gchar *real_name;

        real_name = g_get_real_name ();
        if (strcmp (real_name, "Unknown") == 0) {
                real_name = g_get_user_name ();
        }

        return g_strdup_printf ("%s's %s", real_name, media_type);
}

static DMAPShare *
create_share (protocol_id_t protocol, DMAPDb *db, DMAPContainerDb *container_db)
{
	char *name;

	DMAPShare *share = NULL;

	g_debug ("Initializing %s sharing", protocol_map[protocol]);
	if (protocol == DAAP) {
		name = share_name ? g_strdup (share_name) : default_share_name ("Media");
		share = DMAP_SHARE (daap_share_new (name, music_password, db, container_db, transcode_mimetype));
	} else if (protocol == DPAP) {
		name = share_name ? g_strdup (share_name) : default_share_name ("Pictures");
		share = DMAP_SHARE (dpap_share_new (name, picture_password, db, container_db, transcode_mimetype));
	} else {
		g_error ("Unknown share type");
	}

	g_free (name);

	return share;
}

static void
drop_root (const char *user, const char *group) {
	if (group) {
		struct group *gr;
		if (!(gr = getgrnam(group))) {	
			g_error ("Failed to find group %s", group);
		}

		if (setgid (gr->gr_gid) < 0) {
			g_error ("Failed to set gid");
		}
	}

	if (user) {
		struct passwd *pw;
		if (!(pw = getpwnam(user))) {
			g_error ("Failed to find user %s", user);
		}

		if (setuid (pw->pw_uid) < 0) {
			g_error ("Failed to set uid");
		}
	}
}

static void
daemonize (void)
{
	int child, fd;
	char *pid;

	child = fork ();

	if (child < 0)
		g_error ("Error forking");
	
	if (child > 0)
		exit (EXIT_SUCCESS);
	
	setsid ();

	chdir (DEFAULT_RUNDIR);

	fd = open (lockpath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	if (lockf (fd, F_TLOCK, 0) < 0)
		g_error ("Error opening lock file %s (does it already exist?)", lockpath);
	
	fd = open (pidpath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	pid = g_strdup_printf ("%d", getpid ());
	write (fd, pid, strlen (pid));
	g_free (pid);

	if (user != NULL ||  group != NULL) {
		drop_root (user, group);
	}
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

static DMAPShare *
serve (protocol_id_t protocol,
       DMAPRecordFactory *factory,
       GSList *media_dirs,
       GSList *acceptable_formats)
{
	GSList *l;
	DMAPDb *db;
	DMAPShare *share;
	DbBuilder *builder;
	DMAPContainerDb *container_db;

	gchar *db_protocol_dir = g_strconcat (db_dir, "/", protocol_map[protocol], NULL);
	g_assert (db_module);

	db = DMAP_DB (object_from_module (TYPE_DMAPD_DMAP_DB, 
	                                  module_dir,
					  db_module,
					  "db-dir",
					  db_protocol_dir,
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
		if (enable_dir_containers) {
			db_builder_build_db_starting_at (builder, l->data, db, container_db, NULL);
		} else {
			db_builder_build_db_starting_at (builder, l->data, db, NULL, NULL);
		}
	}

	if (protocol == DAAP && transcode_mimetype && ! enable_rt_transcode)
		dmap_db_foreach (db,
		                (GHFunc) transcode_cache,
		               &(db_dir_and_target_transcode_mimetype_t) { db_protocol_dir, transcode_mimetype });

	loop = g_main_loop_new (NULL, FALSE);
	share = create_share (protocol, DMAP_DB (db), DMAP_CONTAINER_DB (container_db));

	/* FIXME:
	g_object_unref (db);
	g_object_unref (container_db);
	g_object_unref (builder);
	*/
	g_free (db_protocol_dir);

	return share;
}

static gchar *
key_file_s_or_default (GKeyFile *f, char *g, char *k, char *def)
{
	return g_key_file_get_string (f, g, k, NULL) ? g_key_file_get_string (f, g, k, NULL) : def;
}

static gboolean
key_file_b_or_default (GKeyFile *f, char *g, char *k, gboolean def)
{
	return g_key_file_get_boolean (f, g, k, NULL) ? g_key_file_get_boolean (f, g, k, NULL) : def;
}

static void
read_keyfile (void)
{
	GError *error = NULL;
	GKeyFile *keyfile;
	gchar **value;
	gint i;
	gsize len;

	keyfile = g_key_file_new ();

	if (!g_key_file_load_from_file (keyfile, config_file, G_KEY_FILE_NONE, &error)) {
		g_debug ("Could not read configuration file %s: %s", config_file, error->message);
	} else {
		db_dir                = key_file_s_or_default (keyfile, "General", "Database-Dir", db_dir);
		share_name            = key_file_s_or_default (keyfile, "General", "Share-Name", share_name);
		user                  = key_file_s_or_default (keyfile, "General", "User", user);
		group                 = key_file_s_or_default (keyfile, "General", "Group", group);
		enable_dir_containers = key_file_b_or_default (keyfile, "General", "Dir-Containers", enable_dir_containers);
		transcode_mimetype    = key_file_s_or_default (keyfile, "Music", "Transcode-Mimetype", transcode_mimetype);
		enable_rt_transcode   = key_file_b_or_default (keyfile, "Music", "Realtime-Transcode", enable_rt_transcode);
		music_password        = key_file_s_or_default (keyfile, "Music", "Password", music_password);
		picture_password      = key_file_s_or_default (keyfile, "Picture", "Password", picture_password);

		value = g_key_file_get_string_list (keyfile, "Music", "Dirs", &len, NULL);
		for (i = 0; i < len; i++)
			add_to_opt_list ("-m", value[i], NULL, &error);

		value = g_key_file_get_string_list (keyfile, "Music", "Acceptable-Formats", &len, NULL);
		for (i = 0; i < len; i++)
			add_to_opt_list ("-M", value[i], NULL, &error);

		value = g_key_file_get_string_list (keyfile, "Picture", "Dirs", &len, NULL);
		for (i = 0; i < len; i++)
			add_to_opt_list ("-p", value[i], NULL, &error);

		value = g_key_file_get_string_list (keyfile, "Picture", "Acceptable-Formats", &len, NULL);
		for (i = 0; i < len; i++)
			add_to_opt_list ("-p", value[i], NULL, &error);
	}

	g_key_file_free (keyfile);
}

static void sigterm_handler (int i)
{
	signal (SIGTERM, sigterm_handler);

	g_debug ("Received TERM signal");

	g_main_loop_quit (loop);
}

static gboolean
dacp_add_guid_cb (DACPShare *share, gchar *guid, gpointer user_data)
{
	// FIXME: handle multiple remotes? See also defn of _guid.
	_guid = g_strdup (guid);
	return TRUE;
}

static gboolean
dacp_lookup_guid_cb (DACPShare *share, gchar *guid, gpointer user_data)
{
	g_debug ("Comparing %s to %s", _guid, guid);
	return _guid && ! strcmp (_guid, guid);
}

static void
dacp_remote_found_cb (DACPShare *share, gchar *service_name, gchar *display_name, gpointer user_data)
{
	// FIXME:
	g_print ("Enter passcode: ");
	gchar passcode[5];

	scanf ("%s", passcode);

	dacp_share_pair (share, service_name, passcode);
}

static void
dacp_player_updated_cb (DACPPlayer *player, DACPShare *share)
{
	dacp_share_player_updated (share);
}

static void
raop_service_added_cb (DMAPMdnsBrowser *browser, DMAPMdnsBrowserService *service, workers_t *workers)
{
	gchar *host = NULL;

	g_debug ("service added %s:%s:%s:%d", service->service_name, service->name, service->host, service->port);

	g_object_get (workers->av_render, "host", &host, NULL);

	if (host == NULL) {
		g_warning ("RAOP host not set");
	} else if (workers->dacp_share != NULL) {
		g_debug ("DACP share already started, not doing it again");	
	} else {
		if (strcmp (service->host, host)) {
			g_debug ("Wrong host, %s does not match %s", service->host, host);
		} else {
			DMAPDb *db;
			DMAPContainerDb *container_db;

			g_object_set (workers->av_render, "port", service->port, NULL);
			g_object_set (workers->av_render, "transport-protocol", service->transport_protocol, NULL);

			// FIXME: set other properties (protocol, generation) from mDNS!
			
			g_object_get (workers->daap_share, "db", &db, NULL); // FIXME: decompose share we can use db without DAAP.
			g_object_get (workers->daap_share, "container-db", &container_db, NULL);
			workers->dacp_share = dacp_share_new ("FIXME",
			                                      DACP_PLAYER (workers->av_render),
							      db,
							      container_db);

			g_signal_connect_object (workers->dacp_share, "add-guid", G_CALLBACK (dacp_add_guid_cb), NULL, 0);
			g_signal_connect_object (workers->dacp_share, "lookup-guid", G_CALLBACK (dacp_lookup_guid_cb), NULL, 0);
			g_signal_connect_object (workers->dacp_share, "remote-found", G_CALLBACK (dacp_remote_found_cb), NULL, 0);

			g_signal_connect_object (workers->av_render, "player-updated", G_CALLBACK (dacp_player_updated_cb), workers->dacp_share, 0);

			dacp_share_start_lookup (workers->dacp_share);

			// FIXME: this is to test, remove
			//GList *list = NULL;
			//list = g_list_prepend (list, dmap_db_lookup_by_id (db, G_MAXINT));
			//list = g_list_prepend (list, dmap_db_lookup_by_id (db, G_MAXINT - 1));
			//dacp_player_cue_play(DACP_PLAYER (workers->av_render), list, 0);
		}
	}
}

int main (int argc, char *argv[])
{
	int exitval = EXIT_SUCCESS;
	GError *error = NULL;
	GOptionContext *context;
	AVMetaReader *av_meta_reader = NULL;
	PhotoMetaReader *photo_meta_reader = NULL;

	workers_t workers = { NULL, NULL, NULL, NULL };

	stringleton_init ();

	loop = g_main_loop_new (NULL, FALSE);
	if (NULL == loop) {
		g_error ("Could not allocate event loop");
	}

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

	av_render_module = getenv ("DMAPD_AV_RENDER_MODULE");
	av_render_module = av_render_module ? av_render_module : DEFAULT_AV_RENDER_MOD;

	photo_meta_reader_module = getenv ("DMAPD_PHOTO_META_READER_MODULE");
	photo_meta_reader_module = photo_meta_reader_module ? photo_meta_reader_module : DEFAULT_PHOTO_META_READER_MOD;

	db_module = getenv ("DMAPD_DB_MODULE");
	db_module = db_module ? db_module : DEFAULT_DB_MOD;

	// This must be before read_keyfile ().
	config_file = getenv ("DMAPD_CONFIG_FILE");
	config_file = config_file ? config_file : DEFAULT_CONFIG_FILE;

	context = g_option_context_new ("-m | -p: serve media using DMAP");
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

	if (strcmp (av_render_module, "null") != 0) {
		GOptionGroup *group;
		GHashTable *options = g_hash_table_new (g_str_hash, g_str_equal);
		gchar *mod = parse_plugin_option (av_render_module, options);
		workers.av_render = AV_RENDER (object_from_module (TYPE_AV_RENDER,
		                                                   module_dir,
								   mod,
								   NULL));
		g_object_set (workers.av_render, "host", g_hash_table_lookup (options, "host"), NULL);
		if (workers.av_render) {
			group = av_render_get_option_group (workers.av_render);
			if (group)
				g_option_context_add_group (context, group);
		}
		g_hash_table_destroy (options);
	}

	if (strcmp (photo_meta_reader_module, "null") != 0) {
		GOptionGroup *group;
		photo_meta_reader = PHOTO_META_READER (object_from_module (TYPE_PHOTO_META_READER,
									   module_dir,
		                                                           photo_meta_reader_module,
									   NULL));
		if (photo_meta_reader) {
			group = photo_meta_reader_get_option_group (photo_meta_reader);
			if (group)
				g_option_context_add_group (context, group);
		}
	}

	read_keyfile ();

	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_error ("Option parsing failed: %s", error->message);
	}

	if (enable_version) {
		g_print ("dmapd version %s\n", VERSION);
		exit (EXIT_SUCCESS);
	}

	g_object_set (photo_meta_reader, "max-thumbnail-width", max_thumbnail_width, NULL);

	if (! (av_meta_reader || photo_meta_reader)) {
		g_error ("Neither an AV or photograph metadata reader plugin could be loaded");
	}

	if (! av_meta_reader) {
		if (music_dirs) {
			g_error ("Could not load any AV metadata reader plugin but music directory provided");
		} else {
			g_warning ("Could not load any AV metadata reader plugin");
		}
	}

	if (! photo_meta_reader) {
		if (picture_dirs) {
			g_error ("Could not load any photograph metadata reader plugin but photograph directory provided");
		} else {
			g_warning ("Could not load any photograph metadata reader plugin");
		}
	}

	if (! (music_dirs || picture_dirs || enable_render)) {
		g_print ("%s", g_option_context_get_help (context, TRUE, NULL));
		g_print ("Must provide '-m', '-p' or '-o' option\n");
		exit(EXIT_FAILURE);
	}

	g_option_context_free (context);

	if (! enable_foreground) {
		daemonize ();
	}

	signal (SIGTERM, sigterm_handler);

	if (music_dirs && av_meta_reader) {
#if WITH_DAAP
		if (av_meta_reader == NULL)
			g_error ("Music directory specified but AV metadata reader module is 'null'");
		DMAPRecordFactory *factory;
		factory = DMAP_RECORD_FACTORY (
				g_object_new (
					TYPE_DMAPD_DAAP_RECORD_FACTORY,
					"meta-reader",
					av_meta_reader,
					NULL));
		workers.daap_share = DAAP_SHARE (serve (DAAP, factory, music_dirs, music_formats));
#else
		g_error ("DAAP support not present");
#endif
	}

	if (picture_dirs && photo_meta_reader) {
#ifdef WITH_DPAP
		if (photo_meta_reader == NULL)
			g_error ("Photo directory specified but photo metadata reader module is 'null'");
		DMAPRecordFactory *factory;
		factory = DMAP_RECORD_FACTORY (
				g_object_new (
					TYPE_DMAPD_DPAP_RECORD_FACTORY,
					"meta-reader",
					photo_meta_reader,
					NULL));
		workers.dpap_share = DPAP_SHARE (serve (DPAP, factory, picture_dirs, picture_formats));
#else
		g_error ("DPAP support not present");
#endif
	}

	if (enable_render && workers.av_render) {
#ifdef WITH_DACP
		GError *error = NULL;
		DMAPMdnsBrowser *browser = dmap_mdns_browser_new (DMAP_MDNS_BROWSER_SERVICE_TYPE_RAOP);
		if (browser == NULL) {
			g_error ("Error creating mDNS browser");
		}
		g_signal_connect (G_OBJECT (browser), "service-added", G_CALLBACK (raop_service_added_cb), &workers);
		dmap_mdns_browser_start (browser, &error);
		if (error) {
		        g_error ("error starting browser. code: %d message: %s", error->code, error->message);
		}
#else
		g_error ("DACP support not present");
#endif
	}

	if (! exit_after_loading) {
		g_main_loop_run (loop);
	}

_done:
	if (NULL != loop) {
		g_main_loop_unref (loop);
	}

	if (NULL != workers.daap_share) {
		g_object_unref (workers.daap_share);
	}

	if (NULL != workers.dpap_share) {
		g_object_unref (workers.dpap_share);
	}

	if (NULL != workers.dacp_share) {
		g_object_unref (workers.dacp_share);
	}

	if (NULL != av_meta_reader) {
		g_object_unref (av_meta_reader);
	}
	if (NULL != workers.av_render) {
		g_object_unref (workers.av_render);
	}
	if (NULL != photo_meta_reader) {
		g_object_unref (photo_meta_reader);
	}

	free_globals ();
	stringleton_deinit ();
	g_debug ("Parent Exiting");

	exit(exitval);
}
