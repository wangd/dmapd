/*
 * GStreamer Audio / Video rendering functions.
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

#include <gst/gst.h>

#include "av-render-gst.h"
#include "util-gst.h"

struct AVRenderGstPrivate {
	GMainLoop *loop;
	SoupURI *sink_uri;	// Captures sink plugin and properties.
				// e.g., gst://apexsink?host=foo,port=5000
	GstElement *pipeline;
	GstElement *src_decoder;
	GstElement *resample;
	GstElement *sink;

	GList *song_list;	// Songs to play.
	GList *song_current;	// Song currently playing.
};

enum {
        PROP_0,
	PROP_PLAYING_TIME,
	PROP_SHUFFLE_STATE,
	PROP_REPEAT_STATE,
	PROP_PLAY_STATE,
	PROP_VOLUME
};

DAAPRecord *
av_render_gst_now_playing_record (DACPPlayer * player)
{
	DAAPRecord *fnval;
	AVRenderGst *render = AV_RENDER_GST (player);

	if (render->priv->song_current == NULL) {
		fnval = NULL;
	} else {
		fnval = DAAP_RECORD (render->priv->song_current->data);
	}

	return fnval;
}

const guchar *
av_render_gst_now_playing_artwork (DACPPlayer * player, guint width,
				 guint height)
{
	g_error ("Not implemented");
}

void
av_render_gst_play_pause (DACPPlayer * player)
{
	AVRenderGst *render = AV_RENDER_GST (player);

	if (render->priv->pipeline) {
		GstState state;
		GstStateChangeReturn ret = gst_element_get_state (render->priv->pipeline, &state, NULL,  GST_CLOCK_TIME_NONE);
		if (ret == GST_STATE_CHANGE_SUCCESS) {
			if (state == GST_STATE_PAUSED) {
				transition_pipeline (render->priv->pipeline, GST_STATE_PLAYING);
			} else {
				transition_pipeline (render->priv->pipeline, GST_STATE_PAUSED);
			}
		}
	}
}

void
av_render_gst_pause (DACPPlayer * player)
{
	transition_pipeline (AV_RENDER_GST (player)->priv->pipeline, GST_STATE_PAUSED);
}

// FIXME EOS handler goes to next song; should this be more explicit here?
void play_list_starting_at_current (AVRenderGst *render)
{
	gchar *location;

	transition_pipeline (render->priv->pipeline, GST_STATE_READY);

	g_object_get (render->priv->song_current->data, "location", &location, NULL);
	g_object_set (G_OBJECT (render->priv->src_decoder), "uri", location, NULL);

	g_debug ("Playing %s", location);

	transition_pipeline (render->priv->pipeline, GST_STATE_PLAYING);
}

void
av_render_gst_next_item (DACPPlayer * player)
{
	AVRenderGst *render = AV_RENDER_GST (player);

	render->priv->song_current = render->priv->song_current->next;
	play_list_starting_at_current (render);
}

void
av_render_gst_prev_item (DACPPlayer * player)
{
	g_error ("Not implemented");
}

static void av_render_gst_reset (AVRenderGst *render)
{
        render->priv->pipeline    = NULL;
	render->priv->src_decoder = NULL;
	render->priv->resample    = NULL;
	render->priv->sink        = NULL;
}

void
av_render_gst_cue_clear (DACPPlayer * player)
{
	AVRenderGst *render = AV_RENDER_GST (player);

	if (render->priv->song_list) {
		g_list_free (render->priv->song_list);
	}

	render->priv->song_list    = NULL;
	render->priv->song_current = NULL;

	if (render->priv->pipeline) {
		g_idle_add ((GSourceFunc) g_main_loop_quit, render->priv->loop);
		transition_pipeline (render->priv->pipeline, GST_STATE_NULL);
		gst_object_unref (render->priv->pipeline);
		av_render_gst_reset (render);
	}
}

/* FIXME: mostly copied from AVReadMetaGstPrivate */
static void
pad_added_cb (GstElement *decodebin, GstPad *pad,
              AVRenderGstPrivate *priv)
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
        } else if (g_strrstr (mimetype, "audio")) {
                GstPad *sink_pad;

                sink_pad = gst_element_get_static_pad (priv->resample, "sink");
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

gboolean bus_cb (GstBus *bus, GstMessage *message, AVRenderGst *render)
{
	switch (message->type) {
	case GST_MESSAGE_ERROR:
		g_warning ("GStreamer error message");
		transition_pipeline (render->priv->pipeline, GST_STATE_READY);
		g_idle_add ((GSourceFunc) g_main_loop_quit, render->priv->loop);
		break;
	case GST_MESSAGE_EOS:
		g_debug ("GStreamer EOS message");
		if (render->priv->song_current && render->priv->song_current->next) {
			av_render_gst_next_item (DACP_PLAYER (render));
		} else {
			g_idle_add ((GSourceFunc) g_main_loop_quit, render->priv->loop);
		}
		break;
	case GST_MESSAGE_STATE_CHANGED:
		g_debug ("GStreamer state changed message");
		break;
	case GST_MESSAGE_TAG:
		g_debug ("GStreamer tag message");
		break;
	default:
		g_debug ("Unhandled GStreamer message");
	}

	return TRUE;
}

/* FIXME: would like to combine with av-meta-reader-gst.c's, but this needs audioresample
 * and audioconvert.
 */
GstElement *
setup_pipeline (const char *sinkname)
{
	GstElement *pipeline, *src_decoder, *resample, *convert, *sink;

	/* Set up pipeline. */
	pipeline = gst_pipeline_new ("pipeline");

	src_decoder = gst_element_factory_make ("uridecodebin", "src-decoder");
	resample = gst_element_factory_make ("audioresample", "resample");
	convert = gst_element_factory_make ("audioconvert", "convert");
	sink = gst_element_factory_make (sinkname, "sink");

	if (pipeline == NULL || src_decoder == NULL || resample == NULL || convert == NULL || sink == NULL) {
		g_warning ("Error creating a GStreamer pipeline");
		goto _error;
	}

	gst_bin_add_many (GST_BIN (pipeline),
			  src_decoder,
			  resample,
			  convert,
			  sink,
			  NULL);

	if (gst_element_link (resample, convert) == FALSE) {
		g_warning ("Error linking GStreamer pipeline");
		goto _error;
	}

	if (gst_element_link (convert, sink) == FALSE) {
		g_warning ("Error linking GStreamer pipeline");
		goto _error;
	}

	g_debug ("Pipeline complete");
	return pipeline;

_error:
	if (src_decoder != NULL)
		g_object_unref (src_decoder);
	if (resample != NULL)
		g_object_unref (resample);
	if (convert != NULL)
		g_object_unref (convert);
	if (sink != NULL)
		g_object_unref (sink);

	return NULL;
}

void
av_render_gst_cue_play (DACPPlayer * player, GList * records, guint index)
{
	AVRenderGst *render = AV_RENDER_GST (player);

	if (! (render->priv->pipeline = setup_pipeline ("apexsink")))
		goto _return;

	render->priv->src_decoder      = gst_bin_get_by_name (GST_BIN (render->priv->pipeline), "src-decoder");
	render->priv->resample = gst_bin_get_by_name (GST_BIN (render->priv->pipeline), "resample");
	render->priv->sink     = gst_bin_get_by_name (GST_BIN (render->priv->pipeline), "sink");
	GstBus *bus            = gst_pipeline_get_bus (GST_PIPELINE (render->priv->pipeline));

	if (render->priv->src_decoder == NULL
	 || render->priv->resample == NULL
	 || render->priv->sink == NULL
         || bus == NULL)
		goto _return;

	g_signal_connect (render->priv->src_decoder,
                          "pad-added",
                          G_CALLBACK (pad_added_cb),
                          render->priv);

	gst_bus_add_watch (bus, (GstBusFunc) bus_cb, render);
		
	// HOLY COW, IS A URI WHAT I WANT TO PARSE?
	// FIXME:
	g_object_set (G_OBJECT (render->priv->sink), "host", "Dragon.local", NULL);
	// FIXME:
	g_object_set (G_OBJECT (render->priv->sink), "port", 5000, NULL);
	// FIXME:
	g_object_set (G_OBJECT (render->priv->sink), "generation", 1, NULL);
	// FIXME:
	g_object_set (G_OBJECT (render->priv->sink), "transport-protocol", 1, NULL);

	render->priv->song_list = records;
	render->priv->song_current = g_list_nth (records, index);

	play_list_starting_at_current (AV_RENDER_GST (player));

	g_main_loop_run (render->priv->loop);

	if (transition_pipeline (render->priv->pipeline, GST_STATE_NULL) == FALSE)
		goto _return;

_return:
	gst_object_unref (render->priv->pipeline);
	av_render_gst_reset (render);
}

GOptionGroup *av_render_gst_get_option_group (AVRender *render)
{
	if (gst_is_initialized ()) {
		return NULL;
	} else {
		return gst_init_get_option_group ();
	}
}

static void av_render_gst_register_type (GTypeModule *module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule *module)
{
        av_render_gst_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT gboolean
dmapd_module_unload (GTypeModule *module)
{
	return TRUE;
}

static void
av_render_gst_init (AVRenderGst *render)
{
	render->priv = AV_RENDER_GST_GET_PRIVATE (render);

	render->priv->loop = g_main_loop_new (NULL, FALSE);
	render->priv->song_list = NULL;
	render->priv->song_current = NULL;

	av_render_gst_reset (render);
}

static void
av_render_gst_finalize (GObject *self)
{
}

static void
av_render_gst_class_finalize (AVRenderGstClass *klass)
{
}

static void
av_render_gst_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
        switch (prop_id) {
                case PROP_PLAYING_TIME:
			g_error ("get prop");
                        break;
                case PROP_SHUFFLE_STATE:
			g_error ("get prop");
                        break;
                case PROP_REPEAT_STATE:
			g_error ("get prop");
                        break;
                case PROP_PLAY_STATE:
			g_error ("get prop");
                        break;
                case PROP_VOLUME:
			g_error ("get prop");
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
av_render_gst_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
        switch (prop_id) {
                case PROP_PLAYING_TIME:
			g_error ("set prop");
                        break;
                case PROP_SHUFFLE_STATE:
			g_error ("set prop");
                        break;
                case PROP_REPEAT_STATE:
			g_error ("set prop");
                        break;
                case PROP_VOLUME:
			g_error ("set prop");
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
av_render_gst_class_init (AVRenderGstClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AVRenderClass *av_render_class = AV_RENDER_CLASS (klass);

	g_type_class_add_private (klass, sizeof (AVRenderGstPrivate));

	gobject_class->set_property = av_render_gst_set_property;
	gobject_class->get_property = av_render_gst_get_property;
	gobject_class->finalize = av_render_gst_finalize;

	av_render_class->now_playing_record = av_render_gst_now_playing_record;
	av_render_class->now_playing_artwork = av_render_gst_now_playing_artwork;
	av_render_class->play_pause = av_render_gst_play_pause;
	av_render_class->pause = av_render_gst_pause;
	av_render_class->next_item = av_render_gst_next_item;
	av_render_class->prev_item = av_render_gst_prev_item;
	av_render_class->cue_clear = av_render_gst_cue_clear;
	av_render_class->cue_play = av_render_gst_cue_play;
	av_render_class->get_option_group = av_render_gst_get_option_group;

	g_object_class_override_property (gobject_class, PROP_PLAYING_TIME, "playing-time");
	g_object_class_override_property (gobject_class, PROP_SHUFFLE_STATE, "shuffle-state");
	g_object_class_override_property (gobject_class, PROP_REPEAT_STATE, "repeat-state");
	g_object_class_override_property (gobject_class, PROP_PLAY_STATE, "play-state");
	g_object_class_override_property (gobject_class, PROP_VOLUME, "volume");
}

G_DEFINE_DYNAMIC_TYPE (AVRenderGst,
                       av_render_gst,
                       TYPE_AV_RENDER)
