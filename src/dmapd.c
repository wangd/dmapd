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
#include <glib/gprintf.h>
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

#define CONFFILE SYSCONFDIR           "/dmapd.conf"
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

GMainLoop *loop;

static GSList *music_dirs              = NULL;
static GSList *picture_dirs            = NULL;
static gchar *db_dir                   = NULL;
static gchar *lockpath                 = NULL;
static gchar *pidpath                  = NULL;
static gchar *user                     = NULL;
static gchar *group                    = NULL;
static gboolean render                 = FALSE;
static gboolean foreground             = FALSE;
static gchar *share_name               = NULL;
static gchar *transcode_mimetype       = NULL;
static gboolean rt_transcode           = FALSE;
static gchar *db_module                = NULL;
static gchar *av_meta_reader_module    = NULL;
static gchar *av_render_module         = NULL;
static gchar *photo_meta_reader_module = NULL;
static guint  max_thumbnail_width      = 128;

// FIXME: make non-global, support mult. remotes and free when done.
// store persistently or set in config file?
static gchar *_guid = NULL;

static void
free_globals (void)
{
	g_debug ("Free'ing globals");

	slist_deep_free (music_dirs);
	slist_deep_free (picture_dirs);
}

static gboolean
add_dir (const gchar *option_name,
	 const gchar *value,
	 gpointer data,
	 GError **error)
{
	if (strcmp (option_name, "-m") == 0) {
		music_dirs = g_slist_append (music_dirs, g_strdup (value));
	} else if (strcmp (option_name, "-p") == 0) {
		picture_dirs = g_slist_append (picture_dirs, g_strdup (value));
	} else {
		g_error ("Unsupported directory option: %s.", option_name);
	}
	return TRUE;
}

/* FIXME: how to enumerate available transcoding formats? */
static GOptionEntry entries[] = {
	{ "foreground", 'f', 0, G_OPTION_ARG_NONE, &foreground, "Do not fork; remain in foreground", NULL },
	{ "name", 'n', 0, G_OPTION_ARG_STRING, &share_name, "Name of media shares", NULL },
	{ "music-dir", 'm', 0, G_OPTION_ARG_CALLBACK, add_dir, "Music directory", NULL },
	{ "picture-dir", 'p', 0, G_OPTION_ARG_CALLBACK, add_dir, "Picture directory", NULL },
	{ "lockpath", 'l', 0, G_OPTION_ARG_STRING, &lockpath, "Path to lockfile", NULL },
	{ "pidpath", 'i', 0, G_OPTION_ARG_STRING, &pidpath, "Path to PID file", NULL },
	{ "db-dir", 'd', 0, G_OPTION_ARG_STRING, &db_dir, "Media database directory", NULL },
	{ "user", 'u', 0, G_OPTION_ARG_STRING, &user, "User to run as", NULL },
	{ "group", 'g', 0, G_OPTION_ARG_STRING, &group, "Group to run as", NULL },
	{ "render", 'o', 0, G_OPTION_ARG_NONE, &render, "Render using AirPlay", NULL },
	{ "transcode-mimetype", 't', 0, G_OPTION_ARG_STRING, &transcode_mimetype, "Target MIME type for transcoding", NULL },
	{ "rt-transcode", 'r', 0, G_OPTION_ARG_NONE, &rt_transcode, "Perform transcoding in real-time", NULL },
	{ "max-thumbnail-width", 'w', 0, G_OPTION_ARG_INT, &max_thumbnail_width, "Maximum thumbnail size (may reduce memory use)", NULL },
	{ NULL }
};

static char *
default_share_name ()
{
        const gchar *real_name;

        real_name = g_get_real_name ();
        if (strcmp (real_name, "Unknown") == 0) {
                real_name = g_get_user_name ();
        }

        return g_strdup_printf ("%s's Media", real_name);
}

static DMAPShare *
create_share (protocol_id_t protocol, DMAPDb *db, DMAPContainerDb *container_db)
{
	char *name;

	DMAPShare *share = NULL;

	name = share_name ? share_name : default_share_name ();

	g_debug ("Initializing %s sharing", protocol_map[protocol]);
	if (protocol == DAAP) {
		share = DMAP_SHARE (daap_share_new (name, NULL, db, container_db, transcode_mimetype));
	} else if (protocol == DPAP) {
		share = DMAP_SHARE (dpap_share_new (name, NULL, db, container_db, transcode_mimetype));
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
	char *_lockpath, *pid, *_pidpath;

	if (getppid () == 1)
		return; /* Already a daemon */

	child = fork ();

	if (child < 0)
		g_error ("Error forking");
	
	if (child > 0)
		exit (EXIT_SUCCESS);
	
	setsid ();

	chdir (RUNDIR);

	_lockpath = lockpath ? lockpath : LOCKPATH;
	fd = open (_lockpath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	if (lockf (fd, F_TLOCK, 0) < 0)
		g_error ("Error opening lock file %s (does it already exist?)", _lockpath);
	
	_pidpath = pidpath ? pidpath : RUNDIR "/dmapd.pid";
	fd = open (_pidpath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

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
	g_printf ("%s\n", message);
}

static void
debug_null (const char *log_domain,
	    GLogLevelFlags log_level,
	    const gchar *message,
	    gpointer user_data)
{
}

static void
do_transcode (DAAPRecord *record, gchar *cachepath)
{
	GError *error = NULL;
	GInputStream *stream, *decoded_stream;
	
	stream = daap_record_read (record, &error);
	if (! stream) {
		gchar *location = NULL;
		g_object_get (record, "location", &location, NULL);
		g_warning ("Error opening %s: %s", location, error->message);
		g_error_free (error);
		goto _return_no_close;
	}
	/* FIXME: make target format flexible: */
	decoded_stream = dmap_gst_input_stream_new ("audio/mp3", stream);
	if (! decoded_stream) {
		gchar *location;
		g_object_get (record, "location", &location, NULL);
		g_warning ("Error opening %s", location);
		g_error_free (error);
		goto _return_no_close_decoded_stream;
	}
	gssize read_size;
	gchar buf[BUFSIZ];

	FILE *outfile = fopen (cachepath, "w");
	if (outfile == NULL) {
		 g_warning ("Error opening: %s", cachepath);
		 goto _return_no_close_outfile;
	}

	/* FIXME: is there a glib function to do this? */
	do {
		read_size = g_input_stream_read (decoded_stream,
						 buf,
						 BUFSIZ,
						 NULL,
						&error);
		if (read_size > 0) {
			if (fwrite (buf, 1, read_size, outfile) != read_size) {
				 g_warning ("Error writing transcoded data");
				 goto _return;
			}
		} else if (error != NULL) {
			g_warning ("Error transcoding: %s", error->message);
			g_error_free (error);
			goto _return;
		}
	} while (read_size > 0);

_return:
	fclose (outfile);
_return_no_close_outfile:
	g_input_stream_close (decoded_stream, NULL, NULL);
_return_no_close_decoded_stream:
	g_input_stream_close (stream, NULL, NULL); /* FIXME: should this be done in GGstMp3InputStream class? */
_return_no_close:
	return;
}

/* NOTE: This is here and not in the individual DMAPRecords because records
 * have no knowlege of the database, db_dir, etc.
 */
static void
transcode_cache (gpointer id, DAAPRecord *record, gchar *db_dir)
{
	gboolean has_video = FALSE;
	gchar *location = NULL;
	gchar *format = NULL;
	gchar *cacheuri = NULL;
	gchar *cachepath = NULL;

	g_object_get (record,
		     "location",
		     &location,
		      "format",
		     &format,
		     "has-video",
		     &has_video,
		      NULL);

	if (! (location && format)) {
		g_warning ("Error reading record properties for transcoding");
		return;
	}

	/* FIXME: make target format flexible: */
	if (! strcmp (format, "mp3")) {
		g_debug ("Transcoding not necessary %s", location);
		return;
	}

	if (has_video) {
		g_debug ("Not transcoding video %s", location);
		return;
	}

	g_assert (db_dir);
	cachepath = cache_path (CACHE_TYPE_TRANSCODED_DATA, db_dir, location);

	if (! g_file_test (cachepath, G_FILE_TEST_EXISTS)) {
		/* FIXME: return value, not void: */
		g_debug ("Transcoding %s to %s", location, cachepath);
		do_transcode (record, cachepath);
	} else {
		g_debug ("Found transcoded data at %s for %s", cachepath, location);
	}

	/* Replace previous location with URI to transcoded file. */
	cacheuri = g_filename_to_uri(cachepath, NULL, NULL);
	g_object_set (record, "location", cacheuri, NULL);
	g_free (cacheuri);
	/* FIXME: make target format flexible: */
	g_object_set (record, "format", "mp3", NULL);

	g_free (cachepath);

	return;
}

static DMAPShare *
serve (protocol_id_t protocol,
       DMAPRecordFactory *factory,
       GSList *media_dirs)
{
	GSList *l;
	DMAPDb *db;
	DMAPShare *share;
	DbBuilder *builder;
	DMAPContainerDb *container_db;

	gchar *db_protocol_dir = g_strconcat (db_dir, "/", protocol_map[protocol], NULL);
	g_assert (db_module);
	db = DMAP_DB (object_from_module (TYPE_DMAPD_DMAP_DB, db_module, "db-dir", db_protocol_dir, "record-factory", factory, NULL));
	g_assert (db);
	container_db = DMAP_CONTAINER_DB (dmapd_dmap_container_db_new ());
	builder = DB_BUILDER (object_from_module (TYPE_DB_BUILDER, "gdir", NULL));

	for (l = media_dirs; l; l = l->next) {
		db_builder_build_db_starting_at (builder, l->data, db, container_db, NULL);
	}

	if (protocol == DAAP && transcode_mimetype && ! rt_transcode)
		dmap_db_foreach (db, (GHFunc) transcode_cache, db_protocol_dir);

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

static void
read_keyfile (void)
{
	GError *error = NULL;
	GKeyFile *keyfile;
	gchar **dir;
	gint i;
	gsize len;

	keyfile = g_key_file_new ();

	if (!g_key_file_load_from_file (keyfile, CONFFILE, G_KEY_FILE_NONE, &error)) {
		g_debug ("Could not read configuration file %s: %s", CONFFILE, error->message);
	} else {

		db_dir             = g_key_file_get_string  (keyfile, "General", "Database-Dir", NULL);
		share_name         = g_key_file_get_string  (keyfile, "General", "Share-Name", NULL);
		user               = g_key_file_get_string  (keyfile, "General", "User", NULL);
		group              = g_key_file_get_string  (keyfile, "General", "Group", NULL);
		transcode_mimetype = g_key_file_get_string  (keyfile, "Music", "Transcode-Mimetype", NULL);
		rt_transcode       = g_key_file_get_boolean (keyfile, "Music", "Realtime-Transcode", NULL);

		dir = g_key_file_get_string_list (keyfile, "Music", "Dirs", &len, NULL);
		for (i = 0; i < len; i++)
			add_dir ("-m", dir[i], NULL, &error);

		dir = g_key_file_get_string_list (keyfile, "Picture", "Dirs", &len, NULL);
		for (i = 0; i < len; i++)
			add_dir ("-p", dir[i], NULL, &error);
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

	g_debug ("service added %s:%s:%s:%d (%s)", service->service_name, service->name, service->host, service->port);

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

			g_object_set (workers->av_render, "port", service->port);

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

	g_type_init ();
	g_thread_init (NULL);
	stringleton_init ();

	loop = g_main_loop_new (NULL, FALSE);

	if (getenv ("DMAPD_DEBUG") != NULL) {
		g_log_set_handler ("libdmapsharing", G_LOG_LEVEL_DEBUG, debug_printf, NULL);
		g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_printf, NULL);
	} else {
		g_log_set_handler ("libdmapsharing", G_LOG_LEVEL_DEBUG, debug_null, NULL);
		g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_null, NULL);
	}

	av_meta_reader_module = getenv ("DMAPD_AV_META_READER_MODULE");
	av_meta_reader_module = av_meta_reader_module ? av_meta_reader_module : DEFAULT_AV_META_READER_MOD;

	av_render_module = getenv ("DMAPD_AV_RENDER_MODULE");
	av_render_module = av_render_module ? av_render_module : DEFAULT_AV_RENDER_MOD;

	photo_meta_reader_module = getenv ("DMAPD_PHOTO_META_READER_MODULE");
	photo_meta_reader_module = photo_meta_reader_module ? photo_meta_reader_module : DEFAULT_PHOTO_META_READER_MOD;

	db_module = getenv ("DMAPD_DB_MODULE");
	db_module = db_module ? db_module : DEFAULT_DB_MOD;

	context = g_option_context_new ("-m | -p: serve media using DMAP");
	g_option_context_add_main_entries (context, entries, NULL);

	if (strcmp (av_meta_reader_module, "null") != 0) {
		GOptionGroup *group;
		av_meta_reader = AV_META_READER (object_from_module (TYPE_AV_META_READER, av_meta_reader_module, NULL));
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
		workers.av_render = AV_RENDER (object_from_module (TYPE_AV_RENDER, mod, NULL));
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

	if (! (music_dirs || picture_dirs || render)) {
		g_print ("%s", g_option_context_get_help (context, TRUE, NULL));
		g_print ("Must provide '-m', '-p' or '-o' option\n");
		exit(EXIT_FAILURE);
	}

	g_option_context_free (context);

	if (! foreground) {
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
		workers.daap_share = DAAP_SHARE (serve (DAAP, factory, music_dirs));
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
		workers.dpap_share = DPAP_SHARE (serve (DPAP, factory, picture_dirs));
#else
		g_error ("DPAP support not present");
#endif
	}

	if (render && workers.av_render) {
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

	g_main_loop_run (loop);

	if (workers.daap_share)
		g_object_unref (workers.daap_share);

	if (workers.dpap_share)
		g_object_unref (workers.dpap_share);

	if (workers.dacp_share)
		g_object_unref (workers.dacp_share);

	if (av_meta_reader)
		g_object_unref (av_meta_reader);
	if (workers.av_render)
		g_object_unref (workers.av_render);
	if (photo_meta_reader)
		g_object_unref (photo_meta_reader);

	free_globals ();
	stringleton_deinit ();
	g_debug ("Parent Exiting");

	exit(exitval);
}
