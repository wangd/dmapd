/*
 * Utility functions implemented using GStreamer
 *
 * Copyright (C) 2009 W. Michael Petullo <mike@flyn.org>
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

#include <string.h>
#include <gst/gst.h>

#include "util-gst.h"
#include "dmapd-daap-record.h"
#include "av-meta-reader-gst.h"

struct AVMetaReaderGstPrivate {
	GMainLoop *loop;
	GMutex tag_read;
	GstElement *pipeline;
	GstElement *src_decoder;
	GstElement *sink;
	gboolean has_video;
	gboolean pause_successful;
};

static void
av_meta_reader_gst_set_property (GObject *object,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
av_meta_reader_gst_get_property (GObject *object,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static GOptionGroup *
av_meta_reader_gst_get_option_group (AVMetaReader *reader)
{
	if (gst_is_initialized ()) {
		return NULL;
	} else {
		return gst_init_get_option_group ();
	}
}

static void
av_meta_reader_gst_finalize (GObject *self)
{
	//G_OBJECT_CLASS (av_meta_reader_gst_parent_class)->finalize (self);
}

static void
av_meta_reader_gst_class_finalize (AVMetaReaderGstClass *klass)
{
}

static gboolean
message_loop (GstElement *element, GstTagList **tags)
{
	GstBus *bus = NULL;
	GstMessage *message = NULL;
	gboolean fnval = FALSE;

	g_assert (NULL != element);
	g_assert (NULL != tags);

	bus = gst_element_get_bus (element);
	if (NULL == bus) {
		g_warning ("Error getting bus from element");
		goto _return;
	}

	while ((message = gst_bus_pop (bus))) {
		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_TAG: {
			GstTagList *new_tags = NULL;

			gst_message_parse_tag (message, &new_tags);
			if (NULL == new_tags) {
				g_warning ("Error parsing tags");
				goto _return;
			}

			if (*tags) {
				GstTagList *old_tags = *tags;
				*tags = gst_tag_list_merge (old_tags, new_tags, GST_TAG_MERGE_KEEP);

				gst_tag_list_free (old_tags);
				gst_tag_list_free (new_tags);

				if (NULL == *tags) {
					g_warning ("Error merging tag lists");
					goto _return;
				}
			} else {
				*tags = new_tags;
			}
			break;
		}
		default:
			break;
		}
		gst_message_unref (message);
	}

	fnval = TRUE;

_return:
	if (NULL != bus) {
		gst_object_unref (bus);
	}

	return fnval;
}

static gchar *
determine_format (DAAPRecord *record, const gchar *description)
{
	gchar *format;

	if (g_strrstr (description, "MP3"))
		format = "mp3";
	else if (g_strrstr (description, "MPEG-4 AAC"))
		format = "aac";
	else if (g_strrstr (description, "Vorbis"))
		format = "ogg";
	else if (g_strrstr (description, "FLAC"))
		format = "flac";
	else {
		gchar *ext, *location;

		g_debug ("Could not get type from stream, using filename");
		g_object_get (record, "location", &location, NULL);
		ext = strrchr (location, '.');
		if (ext == NULL) {
			g_debug ("Could not get type from filename, guessing");
			ext = "mp3";
		} else {
			ext++;
		}
		format = ext;
	}

	g_debug ("    Format is %s.", format);
	return format;
}


static void
insert_tag (const GstTagList * list, const gchar * tag, DAAPRecord *record)
{
	gint i;

	g_assert (tag);

	for (i = 0; i < gst_tag_list_get_tag_size (list, tag); i++) {
		gchar *val;

		if (gst_tag_get_type (tag) == G_TYPE_STRING) {
			if (!gst_tag_list_get_string_index (list, tag, i, &val))
				g_assert_not_reached ();
		} else {
			val = g_strdup_value_contents (gst_tag_list_get_value_index (list, tag, i));
			if (NULL == val) {
				g_warning ("Could not get value contents");
				goto done;
			}
		}

		g_debug ("    Tag %s is %s.", tag, val);
		if (! strcmp ("title", tag)) {
			g_object_set (record, "title", val, NULL);
		} else if (! strcmp ("artist", tag)) {
			g_object_set (record, "songartist", val, NULL);
		} else if (! strcmp ("album", tag)) {
			g_object_set (record, "songalbum", val, NULL);
		} else if (! strcmp ("disc-number", tag)) {
			errno = 0;
			long disc = strtol (val, NULL, 10);
			if (! errno) {
				g_object_set (record, "disc", disc, NULL);
			} else {
				g_warning ("Error parsing disc: %s", val);
			}
		} else if (! strcmp ("date", tag)) {
			// val should be "1985-01-01."
			if (strlen (val) < 4) {
				g_warning ("Error parsing date: %s", val);
			} else {
				val[4] = 0x00;
				errno = 0;
				long year = strtol (val, NULL, 10);
				if (! errno) {
					g_object_set (record, "year", year, NULL);
				} else {
					g_warning ("Error parsing year: %s", val);
				}
			}
		} else if (! strcmp ("genre", tag)) {
			g_object_set (record, "songgenre", val, NULL);
		} else if (! strcmp ("audio-codec", tag)) {
			gboolean has_video;
			g_object_get (record, "has-video", &has_video, NULL);
			g_debug ("    %s video.", has_video ? "Has" : "Does not have");
			if (has_video) {
				g_object_set (record, "mediakind", DMAP_MEDIA_KIND_MOVIE, NULL);
				/* FIXME: get from video stream. */
				gchar *ext, *location;

				g_object_get (record, "location", &location, NULL);
				ext = strrchr (location, '.');
				if (ext == NULL) {
					ext = "mov";
				} else {
					ext++;
				}
				g_object_set (record, "format", ext, NULL);
			} else {
				g_object_set (record, "mediakind", DMAP_MEDIA_KIND_MUSIC, NULL);
				gchar *format = determine_format (record, val);
				g_assert (format);
				g_object_set (record, "format", format, NULL);
			}
		} else if (! strcmp ("track-number", tag)) {
			errno = 0;
			long track = strtol (val, NULL, 10);
			if (! errno) {
				g_object_set (record, "track", track, NULL);
			} else {
				g_warning ("Error parsing track: %s", val);
			}
		} else {
			g_debug ("    Unused metadata %s.", tag);
		}
		g_free (val);
	}
done:
	return;
}

static gboolean
pause_pipeline (AVMetaReaderGstPrivate *priv)
{
	priv->pause_successful = transition_pipeline (priv->pipeline, GST_STATE_PAUSED);

	/* Run once. */
	return FALSE;
}

static gboolean
quit_mainloop (GMainLoop *loop)
{
	g_idle_add ((GSourceFunc) g_main_loop_quit, loop);

	/* Run once. */
	return FALSE;
}

static void
no_more_pads_cb (GstElement *element, GMainLoop *loop)
{
	g_assert (NULL != loop);

	quit_mainloop (loop);	
}

static void
pad_added_cb (GstElement *decodebin, GstPad *pad, AVMetaReaderGstPrivate *priv)
{
	GstCaps *caps = NULL;
	const gchar *mimetype = NULL;
	GstStructure *structure = NULL;

	g_assert (NULL != decodebin);
	g_assert (NULL != pad);
	g_assert (NULL != priv);

	caps = gst_pad_query_caps (pad, NULL);
	if (NULL == caps || gst_caps_is_empty (caps) || gst_caps_is_any (caps)) {
		g_warning ("Error getting caps from pad");
		goto _return;
	}

	structure = gst_caps_get_structure (caps, 0);
	if (NULL == structure) {
		g_warning ("Error getting structure from caps");
		goto _return;
	}

	mimetype = gst_structure_get_name (structure);
	if (NULL == mimetype) {
		g_warning ("Error getting mimetype from structure");
		goto _return;
	}

	g_debug ("    Added pad with mimetype %s.", mimetype);

	if (g_strrstr (mimetype, "video")) {
		g_debug("Has video component");
		priv->has_video |= TRUE;
	} else if (g_strrstr (mimetype, "audio")) {
		GstPad *sink_pad;

		sink_pad = gst_element_get_static_pad (priv->sink, "sink");
		if (NULL == sink_pad) {
			g_warning ("Error getting static sink pad");
			goto _return;
		}

		if (pads_compatible (pad, sink_pad)) {
			g_assert (! GST_PAD_IS_LINKED (sink_pad));
			gst_pad_link (pad, sink_pad);
		}
		gst_object_unref (sink_pad);
	}

_return:
	if (NULL != caps) {
		gst_caps_unref (caps);
	}
}

static void av_meta_reader_gst_reset (AVMetaReaderGst *reader)
{
	reader->priv->pipeline = NULL;
	reader->priv->src_decoder = NULL;
	reader->priv->sink = NULL;
	reader->priv->has_video = FALSE;
	reader->priv->pause_successful = FALSE;
}

static GstElement *
setup_pipeline (const char *sinkname)
{
	GstElement *pipeline = NULL, *src_decoder = NULL, *sink = NULL;

	g_assert (NULL != sinkname);

	/* Set up pipeline. */
	pipeline    = gst_pipeline_new ("pipeline");
	src_decoder = gst_element_factory_make ("uridecodebin", "src-decoder");
	sink        = gst_element_factory_make (sinkname, "sink");

	if (pipeline == NULL || src_decoder == NULL || sink == NULL) {
		g_warning ("Error creating a GStreamer pipeline");
		goto _done;
	}

	gst_bin_add_many (GST_BIN (pipeline), g_object_ref (src_decoder), g_object_ref (sink), NULL);

	g_debug ("    Created a pipeline.");

_done:
	if (src_decoder != NULL) {
		g_object_unref (src_decoder);
	}

	if (sink != NULL) {
		g_object_unref (sink);
	}

	return pipeline;
}

static gboolean
av_meta_reader_gst_read (AVMetaReader *reader, DAAPRecord *record, const gchar *path)
{
	gchar *uri = NULL;
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 nanoduration;
	GstTagList *tags = NULL;
	AVMetaReaderGst *gst_reader = AV_META_READER_GST (reader);	
	gboolean fnval = FALSE;

	uri = g_filename_to_uri (path, NULL, NULL);
	if (NULL == uri) {
		g_warning ("Error converting %s to URI\n", path);
		goto _return;
	}

	g_mutex_lock (&gst_reader->priv->tag_read);

	g_debug("Processing %s...", uri);

	gst_reader->priv->pipeline = setup_pipeline ("fakesink");
	if (NULL == gst_reader->priv->pipeline) {
		goto _return;
	}

	gst_reader->priv->src_decoder = gst_bin_get_by_name (GST_BIN (gst_reader->priv->pipeline), "src-decoder");
	gst_reader->priv->sink        = gst_bin_get_by_name (GST_BIN (gst_reader->priv->pipeline), "sink");
	
	if (gst_reader->priv->src_decoder == NULL || gst_reader->priv->sink == NULL) {
		g_warning ("Could get src_decoder or sink.");
		goto _return;
	}

	g_object_set (G_OBJECT (gst_reader->priv->src_decoder), "uri", uri, NULL);

	/* Connect callback to identify audio and/or video tracks and link decoder to sink.  */ 
	g_signal_connect (gst_reader->priv->src_decoder,
			  "pad-added",
			  G_CALLBACK (pad_added_cb),
			  gst_reader->priv);

	g_signal_connect (gst_reader->priv->src_decoder,
			  "no-more-pads",
			  G_CALLBACK (no_more_pads_cb),
			  gst_reader->priv->loop);

	/* Run main loop to allow decodebin to create pads. Quit on
	 * "no-more-pads" signal. */
	g_idle_add ((GSourceFunc) pause_pipeline, gst_reader->priv);
	g_timeout_add_seconds (1, (GSourceFunc) quit_mainloop, gst_reader->priv->loop);
	g_main_loop_run (gst_reader->priv->loop);

	if (! gst_element_query_duration (gst_reader->priv->sink, fmt, &nanoduration)) {
		g_warning ("Could not determine duration of %s; skipping", uri);
		goto _return;
	} else {
		g_assert (fmt == GST_FORMAT_TIME);
		/* NOTE: cast avoids segfault on MIPS32: */
		g_object_set (record, "duration", (gint32) (nanoduration / GST_SECOND), NULL);
	}

	if (! message_loop (GST_ELEMENT (gst_reader->priv->pipeline), &tags)) {
		goto _return;
	}

	/* NOTE: Must set has_video before calling insert_tag. */
	g_object_set (record, "has-video", gst_reader->priv->has_video, NULL);

	if (tags) {
		gst_tag_list_foreach (tags, (GstTagForeachFunc) insert_tag, record);
	} else {
		g_warning ("No metadata found for %s", uri);
	}

	fnval = TRUE;

_return:
	if (NULL != gst_reader->priv->pipeline
	 && TRUE  == gst_reader->priv->pause_successful) {
		 if (FALSE == transition_pipeline (gst_reader->priv->pipeline, GST_STATE_NULL)) {
			g_error ("Failed to transition GStreamer state to NULL");
		}
	}

	if (NULL != uri) {
		g_free (uri);
	}

	if (NULL != gst_reader->priv->pipeline) {
		gst_object_unref (gst_reader->priv->pipeline);
	}

	if (NULL != gst_reader->priv->src_decoder) {
		gst_object_unref (gst_reader->priv->src_decoder);
	}

	if (NULL != gst_reader->priv->sink) {
		gst_object_unref (gst_reader->priv->sink);
	}

	if (NULL != tags) {
		gst_tag_list_free (tags);
	}

	av_meta_reader_gst_reset (gst_reader);

	g_mutex_unlock (&gst_reader->priv->tag_read);

	return fnval;
}

static void av_meta_reader_gst_register_type (GTypeModule *module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule *module)
{
	av_meta_reader_gst_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT gboolean
dmapd_module_unload (GTypeModule *module)
{
	return TRUE;
}

static void av_meta_reader_gst_init (AVMetaReaderGst *reader)
{
	reader->priv = AV_META_READER_GST_GET_PRIVATE (reader);

	// FIXME: Never cleared.
	g_mutex_init (&reader->priv->tag_read);

	reader->priv->loop = g_main_loop_new (NULL, FALSE);
	av_meta_reader_gst_reset (reader);
}

static void av_meta_reader_gst_class_init (AVMetaReaderGstClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AVMetaReaderClass *av_meta_reader_class = AV_META_READER_CLASS (klass);

	g_type_class_add_private (klass, sizeof (AVMetaReaderGstPrivate));

	gobject_class->set_property = av_meta_reader_gst_set_property;
	gobject_class->get_property = av_meta_reader_gst_get_property;
	gobject_class->finalize = av_meta_reader_gst_finalize;

	av_meta_reader_class->read = av_meta_reader_gst_read;
	av_meta_reader_class->get_option_group = av_meta_reader_gst_get_option_group;
}

G_DEFINE_DYNAMIC_TYPE (AVMetaReaderGst,
		       av_meta_reader_gst,
		       TYPE_AV_META_READER)
