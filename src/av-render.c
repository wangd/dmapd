/*
 * Audio / Video rendering functions.
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

#include "av-render.h"

static void
av_render_init (AVRender *reader)
{
}

static void
av_render_class_init (AVRenderClass *klass)
{
}

G_DEFINE_TYPE (AVRender, av_render, G_TYPE_OBJECT)

DAAPRecord *
av_render_now_playing_record (DACPPlayer * player)
{
	return AV_RENDER_GET_CLASS (player)->
		now_playing_record (player);
}

const guchar *
av_render_now_playing_artwork (DACPPlayer * player, guint width,
				 guint height)
{
	return AV_RENDER_GET_CLASS (player)->
		now_playing_artwork (player, width, height);
}

void
av_render_play_pause (DACPPlayer * player)
{
	AV_RENDER_GET_CLASS (player)->play_pause (player);
}

void
av_render_pause (DACPPlayer * player)
{
	AV_RENDER_GET_CLASS (player)->pause (player);
}

void
av_render_next_item (DACPPlayer * player)
{
	AV_RENDER_GET_CLASS (player)->next_item (player);
}

void
av_render_prev_item (DACPPlayer * player)
{
	AV_RENDER_GET_CLASS (player)->prev_item (player);
}

void
av_render_cue_clear (DACPPlayer * player)
{
	AV_RENDER_GET_CLASS (player)->cue_clear (player);
}

void
av_render_cue_play (DACPPlayer * player, GList * records, guint index)
{
	AV_RENDER_GET_CLASS (player)->cue_play (player, records, index);
}

GOptionGroup *av_render_get_option_group (AVRender *reader)
{
	return AV_RENDER_GET_CLASS (reader)->get_option_group (reader);
}
