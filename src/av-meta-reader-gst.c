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
	GMutex *tag_read;
	GstElement *pipeline;
	GstElement *src_decoder;
	GstElement *sink;
	gboolean has_video;
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
	GstBus *bus;
	GstMessage *message;

	bus = gst_element_get_bus (element);
	g_return_val_if_fail (bus != NULL, FALSE);
	g_return_val_if_fail (tags != NULL, FALSE);

	while ((message = gst_bus_pop (bus))) {
		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_TAG: {
			GstTagList *new_tags;

			gst_message_parse_tag (message, &new_tags);
			if (*tags) {
				GstTagList *old_tags = *tags;
				*tags = gst_tag_list_merge (old_tags,
							    new_tags,
							    GST_TAG_MERGE_KEEP);
				gst_tag_list_free (old_tags);
				gst_tag_list_free (new_tags);
			} else
				*tags = new_tags;
			break;
		}
		default:
			break;
		}
		gst_message_unref (message);
	}
	gst_object_unref (bus);

	return TRUE;
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

	g_debug ("Format is %s", format);
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
		}

		g_debug ("%s is %s", tag, val);
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
			g_debug ("%s video", has_video ? "Has" : "Does not have");
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
				g_object_set (record, "format", determine_format (record, val), NULL);
			}
		} else if (! strcmp ("track-number", tag)) {
			g_object_set (record, "track", atoi (val), NULL);
		} else {
			g_debug ("Unused metadata");
		}
		g_free (val);
	}
}

static gboolean
pause_pipeline (GstElement *pipeline)
{
	transition_pipeline (pipeline, GST_STATE_PAUSED);

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
	quit_mainloop (loop);	
}

static void
pad_added_cb (GstElement *decodebin, GstPad *pad, 
	      AVMetaReaderGstPrivate *priv)
{
	GstCaps *caps;
	const gchar *mimetype;
	GstStructure *structure;

	caps = gst_pad_get_caps (pad);
	if (gst_caps_is_empty (caps) || gst_caps_is_any (caps)) {
		g_warning ("Error getting caps from pad");
		goto _return;
	}

	structure = gst_caps_get_structure (caps, 0);
	mimetype = gst_structure_get_name (structure);

	g_debug ("Pad mimetype is %s", mimetype);

	if (g_strrstr (mimetype, "video")) {
		g_debug("Has video component");
		priv->has_video |= TRUE;
	} else if (g_strrstr (mimetype, "audio")) {
		GstPad *sink_pad;

		sink_pad = gst_element_get_static_pad (priv->sink, "sink");
		g_assert (sink_pad != NULL);

		if (pads_compatible (pad, sink_pad)) {
			g_assert (! GST_PAD_IS_LINKED (sink_pad));
			gst_pad_link (pad, sink_pad);
		}
		gst_object_unref (sink_pad);
	}
	//g_free (mimetype); /* Mimetype is static, but free'ing it "fixes" leak! */
_return:
	gst_caps_unref (caps);
}

static void av_meta_reader_gst_reset (AVMetaReaderGst *reader)
{
	reader->priv->pipeline = NULL;
	reader->priv->src_decoder = NULL;
	reader->priv->sink = NULL;
	reader->priv->has_video = FALSE;
}

static GstElement *
setup_pipeline (const char *sinkname)
{
	GstElement *pipeline, *src_decoder, *sink;

	/* Set up pipeline. */
	pipeline = gst_pipeline_new ("pipeline");

	src_decoder = gst_element_factory_make ("uridecodebin", "src-decoder");
	sink = gst_element_factory_make (sinkname, "sink");

	if (pipeline == NULL || src_decoder == NULL || sink == NULL) {
		g_warning ("Error creating a GStreamer pipeline");
		goto _error;
	}

	gst_bin_add_many (GST_BIN (pipeline),
			  src_decoder,
			  sink,
			  NULL);

	g_debug ("Pipeline complete");
	return pipeline;

_error:
	if (src_decoder != NULL)
		g_object_unref (src_decoder);
	if (sink != NULL)
		g_object_unref (sink);

	return NULL;
}

static gboolean
av_meta_reader_gst_read (AVMetaReader *reader, DAAPRecord *record, const gchar *path)
{
	gchar *uri = g_strdup_printf ("file://%s", path);
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 nanoduration;
	GstTagList *tags = NULL;
	AVMetaReaderGst *gst_reader = AV_META_READER_GST (reader);	

	g_mutex_lock (gst_reader->priv->tag_read);

	g_debug("Looking at %s", uri);

	if (! (gst_reader->priv->pipeline = setup_pipeline ("fakesink")))
		goto _return;

	gst_reader->priv->src_decoder = gst_bin_get_by_name (GST_BIN (gst_reader->priv->pipeline), "src-decoder");
	gst_reader->priv->sink        = gst_bin_get_by_name (GST_BIN (gst_reader->priv->pipeline), "sink");
	
	if (gst_reader->priv->src_decoder == NULL
	 || gst_reader->priv->sink == NULL)
		goto _return;

	g_object_set (G_OBJECT (gst_reader->priv->src_decoder), "uri", uri, NULL);

	/* Connect callback to identify audio and/or video tracks
	 * and link decoder to sink.
	 */ 
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
	g_idle_add ((GSourceFunc) pause_pipeline, gst_reader->priv->pipeline);
	g_timeout_add_seconds (1,
			      (GSourceFunc) quit_mainloop,
			       gst_reader->priv->loop);
	g_main_loop_run (gst_reader->priv->loop);

	if (! gst_element_query_duration (gst_reader->priv->sink,
					 &fmt,
					 &nanoduration)) {
		g_warning ("Could not determine duration of %s", uri);
	} else {
		g_assert (fmt == GST_FORMAT_TIME);
		/* NOTE: cast avoids segfault on MIPS32: */
		g_object_set (record, "duration", (gint32) (nanoduration / GST_SECOND), NULL);
	}

	if (! message_loop (GST_ELEMENT (gst_reader->priv->pipeline), &tags)) {
		g_warning ("Failed in message reading for %s", uri);
	}

	if (transition_pipeline (gst_reader->priv->pipeline, GST_STATE_NULL) ==
		FALSE) {
		g_error ("Failed to transition GStreamer state to NULL");
	}

	/* NOTE: Must set has_video before calling insert_tag. */
	g_object_set (record, "has-video", gst_reader->priv->has_video, NULL);

	if (tags) {
		gst_tag_list_foreach (tags,
				     (GstTagForeachFunc) insert_tag,
				      record);
		gst_tag_list_free (tags);
		tags = NULL;
	} else {
		g_warning ("No metadata found for %s", uri);
	}

_return:
	g_free (uri);

	gst_object_unref (gst_reader->priv->pipeline);
	av_meta_reader_gst_reset (gst_reader);

	g_mutex_unlock (gst_reader->priv->tag_read);

	return TRUE;
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

	reader->priv->tag_read = g_mutex_new ();

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
