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

#ifndef __AV_RENDER_GST
#define __AV_RENDER_GST

#include <libdmapsharing/dmap.h>

#include "av-render.h"

G_BEGIN_DECLS

#define TYPE_AV_RENDER_GST          (av_render_gst_get_type ())
#define AV_RENDER_GST(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_AV_RENDER_GST, AVRenderGst))
#define AV_RENDER_GST_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), \
                                      TYPE_AV_RENDER_GST, AVRenderGstClass))
#define IS_AV_RENDER_GST(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                      TYPE_AV_RENDER_GST))
#define IS_AV_RENDER_GST_CLASS(k)   (G_TYPE_CHECK_INSTANCE_TYPE ((k), \
                                      TYPE_AV_RENDER_GST))
#define AV_RENDER_GST_GET_CLASS(k)  (G_TYPE_INSTANCE_GET_CLASS ((k), \
                                      TYPE_AV_RENDER_GST, AVRenderGstClass))
#define AV_RENDER_GST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                      TYPE_AV_RENDER_GST, AVRenderGstPrivate))

typedef struct AVRenderGstPrivate AVRenderGstPrivate;

typedef struct _AVRenderGst {
	AVRender parent;
	AVRenderGstPrivate *priv;
} AVRenderGst;

typedef struct _AVRenderGstClass {
        AVRenderClass parent;

	DAAPRecord *(*now_playing_record) (DACPPlayer * player);
	guchar *(*now_playing_artwork) (DACPPlayer * player,
					      guint width, guint height);
	void (*play_pause) (DACPPlayer * player);
	void (*pause) (DACPPlayer * player);
	void (*next_item) (DACPPlayer * player);
	void (*prev_item) (DACPPlayer * player);
	void (*cue_clear) (DACPPlayer * player);
	void (*cue_play) (DACPPlayer * player, GList * records, guint index);
	GOptionGroup *(*get_option_group) (AVRender *render);
} AVRenderGstClass;

GType       av_render_gst_get_type      (void);

DAAPRecord *dacp_player_now_playing_record (DACPPlayer * player);
guchar *dacp_player_now_playing_artwork (DACPPlayer * player,
					       guint width, guint height);
void dacp_player_play_pause (DACPPlayer * player);
void dacp_player_pause (DACPPlayer * player);
void dacp_player_next_item (DACPPlayer * player);
void dacp_player_prev_item (DACPPlayer * player);
void dacp_player_cue_clear (DACPPlayer * player);
void dacp_player_cue_play (DACPPlayer * player, GList * records, guint index);
GOptionGroup *av_render_gst_get_option_group (AVRender *render);

#endif /* __AV_RENDER_GST */

G_END_DECLS
