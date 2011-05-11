/*   FILE: dmapd-stress-test.c -- A stress test client for dmapd
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 03 February 2011 
 *
 * Copyright (c) 2011 W. Michael Petullo <new@flyn.org>
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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include <libdmapsharing/dmap.h>

#include "util.h"
#include "dmapd-dmap-db-ghashtable.h"
#include "dmapd-daap-record-factory.h"
#include "dmapd-dpap-record-factory.h"

enum {
	DAAP,
	DPAP
};

static GMainLoop *loop;

static void
create_connection (const DMAPMdnsBrowserServiceType protocol,
		   const char *name,
		   const char *host,
		   const guint port);

static void
print_record (gpointer id, DMAPRecord *record, gpointer user_data)
{
	if (IS_DAAP_RECORD (record)) {
		gchar *artist = NULL, *title = NULL;

		/* FIXME: print all record properties; need DAAP and DPAP version: */
		g_object_get (record,
			     "songartist", &artist,
			     "title",  &title,
			     NULL);

		g_assert (artist);
		g_assert (title);

		/* FIXME: also actually download media file. */

		g_print ("%d: %s %s\n", GPOINTER_TO_UINT (id), artist, title);

		g_free (artist);
		g_free (title);
	} else if (IS_DPAP_RECORD (record)) {
		gchar *filename = NULL, *format = NULL;

		/* FIXME: print all record properties; need DAAP and DPAP version: */
		g_object_get (record,
			     "filename", &filename,
			     "format",  &format,
			     NULL);

		g_assert (filename);
		g_assert (format);

		/* FIXME: also actually download media file. */

		g_print ("%d: %s %s\n", GPOINTER_TO_UINT (id), filename, format);

		g_free (filename);
		g_free (format);
	} else {
		g_error ("Bad record");
	}
}

static DMAPMdnsBrowserServiceType
get_service_type (DMAPConnection *connection)
{
	if (IS_DAAP_CONNECTION (connection)) {
		return DMAP_MDNS_BROWSER_SERVICE_TYPE_DAAP;
	} else if (IS_DPAP_CONNECTION (connection)) {
		return DMAP_MDNS_BROWSER_SERVICE_TYPE_DPAP;
	} else {
		g_error ("Bad connection");
	}
}

static void
connected_cb (DMAPConnection *connection,
	      gboolean        result,
	      const char     *reason,
	      DMAPDb         *db)
{
	guint port;
	char *name, *host;

	g_print ("Connection cb., DB has %lu entries\n", dmap_db_count (db));

	dmap_db_foreach (db, (GHFunc) print_record, NULL);

	g_object_get (connection,
		     "name", &name,
		     "host", &host,
		     "port", &port,
		      NULL);

	/* Tear down connection. */
	g_object_unref (connection);

	/* Create another connection to same service. */
	create_connection (get_service_type (connection),
			   name,
			   host,
			   port);

	g_free (name);
	g_free (host);
}

static void
create_connection (const DMAPMdnsBrowserServiceType protocol,
		   const char *name,
		   const char *host,
		   const guint port)
{
	DMAPRecordFactory *factory;
	DMAPConnection *conn;
	DMAPDb *db;

	db = DMAP_DB (g_object_new (TYPE_DMAPD_DMAP_DB_GHASHTABLE, NULL));
	if (db == NULL) {
		g_error ("Error creating DB");
	}

	/* FIXME: switch on DMAP_MDNS_BROWSER_SERVICE_TYPE_DAAP, _DPAP, _DACP or _INVALID */
	if (protocol == DMAP_MDNS_BROWSER_SERVICE_TYPE_DAAP) {
		factory = DMAP_RECORD_FACTORY (g_object_new (TYPE_DMAPD_DAAP_RECORD_FACTORY, NULL));
		if (factory == NULL) {
			g_error ("Error creating record factory");
		}
		conn = DMAP_CONNECTION (daap_connection_new (name, host, port, db, factory));
	} else {
		factory = DMAP_RECORD_FACTORY (g_object_new (TYPE_DMAPD_DPAP_RECORD_FACTORY, NULL));
		if (factory == NULL) {
			g_error ("Error creating record factory");
		}
		conn = DMAP_CONNECTION (dpap_connection_new (name, host, port, db, factory));
	}
	dmap_connection_connect (DMAP_CONNECTION (conn), (DMAPConnectionCallback) connected_cb, db);
}

static void
service_added_cb (DMAPMdnsBrowser *browser,
                  DMAPMdnsBrowserService *service,
                  gpointer user_data)
{
	char answer, newline;

	fprintf (stdout,
	    "service added %s:%s:%s:%d (%s)\n",
	     service->service_name,
	     service->name,
	     service->host,
	     service->port,
	     service->password_protected ? "protected" : "not protected");
	fprintf (stdout, "Stress test this service [Y|N]? ");
	fscanf (stdin, "%c%c", &answer, &newline);

	if (answer == 'Y') {
		create_connection (dmap_mdns_browser_get_service_type (browser),
				   service->name,
				   service->host,
				   service->port);
	}
}

static void
debug_null (const char *log_domain,
            GLogLevelFlags log_level,
	    const gchar *message,
	    gpointer user_data)
{
}

int main(int argc, char **argv)
{
	DMAPMdnsBrowser *browserDAAP, *browserDPAP;
	GError *error = NULL;

	g_type_init ();
	g_thread_init (NULL);
	stringleton_init ();

	g_log_set_handler ("libdmapsharing", G_LOG_LEVEL_DEBUG, debug_null, NULL);
	g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_null, NULL);

	loop = g_main_loop_new (NULL, FALSE);

	browserDAAP = dmap_mdns_browser_new (DMAP_MDNS_BROWSER_SERVICE_TYPE_DAAP);

	g_signal_connect (G_OBJECT (browserDAAP),
		         "service-added",
		          G_CALLBACK (service_added_cb),
		          NULL);

	browserDPAP = dmap_mdns_browser_new (DMAP_MDNS_BROWSER_SERVICE_TYPE_DPAP);

	g_signal_connect (G_OBJECT (browserDPAP),
		         "service-added",
		          G_CALLBACK (service_added_cb),
		          NULL);

	g_debug ("starting mdns browsers");

	dmap_mdns_browser_start (browserDAAP, &error);
	if (error) {
		g_warning ("error starting browser. code: %d message: %s",
			    error->code,
			    error->message);
		exit (EXIT_FAILURE);
	}

	dmap_mdns_browser_start (browserDPAP, &error);
	if (error) {
		g_warning ("error starting browser. code: %d message: %s",
			    error->code,
			    error->message);
		exit (EXIT_FAILURE);
	}

	fprintf (stdout, "Waiting for DMAP shares; please run dmapd\n");
	g_main_loop_run (loop);

	exit (EXIT_SUCCESS);
}
