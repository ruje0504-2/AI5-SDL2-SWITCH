/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
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
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <string.h>

#include "nulib.h"
#include "nulib/string.h"
#include "nulib/vector.h"
#include "ai5/mes.h"

struct mes_path_component {
	const char *name;
	const char *name_noargs;
	int nr_children;
	struct mes_path_component **children;
};

#define LEAF(pre, _name) \
	static struct mes_path_component pre##_##_name = { \
		.name = #_name \
	}

#define LEAF2(pre, _name, _name_noargs) \
	static struct mes_path_component pre##_##_name##_##_name_noargs = { \
		.name = #_name, \
		.name_noargs = #_name_noargs \
	}

#define _NODE(linkage, ident, _name, ...) \
	static struct mes_path_component * ident##_children[] = { __VA_ARGS__ }; \
	linkage struct mes_path_component ident = { \
		.name = #_name, \
		.nr_children = ARRAY_SIZE(ident##_children), \
		.children = ident##_children \
	}
#define NODE(...) _NODE(static, __VA_ARGS__)
#define PUBLIC_NODE(...) _NODE(,__VA_ARGS__)

// System.set_font_size
LEAF(sys, set_font_size);

// System.display_number
LEAF(sys, display_number);

// System.Cursor
LEAF(sys_cursor, reload);
LEAF(sys_cursor, unload);
LEAF(sys_cursor, save_pos);
LEAF(sys_cursor, set_pos);
LEAF(sys_cursor, load);
LEAF(sys_cursor, show);
LEAF(sys_cursor, hide);
LEAF(sys_cursor, clear_wheel);
LEAF(sys_cursor, get_wheel);
NODE(sys_cursor_classics, Cursor,
	[0] = &sys_cursor_reload,
	[1] = &sys_cursor_unload,
	[2] = &sys_cursor_save_pos,
	[3] = &sys_cursor_set_pos,
	[4] = &sys_cursor_load,
	[5] = &sys_cursor_show,
	[6] = &sys_cursor_hide,
);
NODE(sys_cursor_isaku, Cursor,
	[0] = &sys_cursor_show,
	[1] = &sys_cursor_hide,
	[2] = &sys_cursor_save_pos,
	[3] = &sys_cursor_set_pos,
	[4] = &sys_cursor_load,
);
NODE(sys_cursor_ai_shimai, Cursor,
	[0] = &sys_cursor_show,
	[1] = &sys_cursor_hide,
	[2] = &sys_cursor_save_pos,
	[3] = &sys_cursor_set_pos,
	[4] = &sys_cursor_load,
	[5] = &sys_cursor_clear_wheel,
	[6] = &sys_cursor_get_wheel,
);
NODE(sys_cursor_allstars, Cursor,
	[0] = &sys_cursor_show,
	[1] = &sys_cursor_hide,
	[2] = &sys_cursor_save_pos,
	[3] = &sys_cursor_set_pos,
	[4] = &sys_cursor_load,
);
NODE(sys_cursor_doukyuusei, Cursor,
	[0] = &sys_cursor_show,
	[1] = &sys_cursor_hide,
	[2] = &sys_cursor_save_pos,
	[3] = &sys_cursor_set_pos,
	[4] = &sys_cursor_load,
);
NODE(sys_cursor_beyond, Cursor,
	[0] = &sys_cursor_show,
	[1] = &sys_cursor_hide,
	[2] = &sys_cursor_save_pos,
	[3] = &sys_cursor_set_pos,
	[4] = NULL,
	[5] = NULL,
	[6] = NULL,
	[7] = NULL,
	[8] = NULL,
	[9] = NULL,
);
NODE(sys_cursor_kakyuusei, Cursor,
	[0] = &sys_cursor_show,
	[1] = &sys_cursor_hide,
	[2] = NULL, // unused
	[3] = &sys_cursor_save_pos,
	[4] = &sys_cursor_set_pos,
	[5] = &sys_cursor_load,
);

// System.Anim
LEAF(sys_anim, init);
LEAF(sys_anim, init2);
LEAF(sys_anim, start);
LEAF(sys_anim, start2);
LEAF(sys_anim, start_sync);
LEAF(sys_anim, start_sync2);
LEAF(sys_anim, stop);
LEAF(sys_anim, stop2);
LEAF(sys_anim, halt);
LEAF(sys_anim, halt2);
LEAF(sys_anim, wait);
LEAF(sys_anim, wait2);
LEAF(sys_anim, wait3);
LEAF(sys_anim, stop_all);
LEAF(sys_anim, halt_all);
LEAF(sys_anim, reset_all);
LEAF(sys_anim, exec_copy_call);
LEAF(sys_anim, halt_group);
LEAF(sys_anim, unpause_all);
LEAF(sys_anim, init_alt);
LEAF(sys_anim, init_from);
LEAF(sys_anim, stream_running);
LEAF(sys_anim, running);
LEAF(sys_anim, wait_all);
NODE(sys_anim, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_halt,
	[5] = &sys_anim_stop_all,
	[6] = &sys_anim_halt_all,
);
NODE(sys_anim_ai_shimai, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_halt,
	[4] = &sys_anim_wait,
	[5] = &sys_anim_stop_all,
	[6] = &sys_anim_halt_all,
	[7] = &sys_anim_reset_all,
	[8] = NULL,
);
NODE(sys_anim_allstars, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_halt,
	[4] = &sys_anim_wait,
	[5] = &sys_anim_stop_all,
	[6] = &sys_anim_halt_all,
	[7] = &sys_anim_reset_all,
	[8] = NULL,
	[9] = NULL,
);
NODE(sys_anim_doukyuusei, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_halt,
	[4] = &sys_anim_wait,
	[5] = &sys_anim_stop_all,
	[6] = &sys_anim_halt_all,
	[7] = &sys_anim_reset_all,
	[8] = &sys_anim_exec_copy_call,
	[9] = &sys_anim_halt_group,
	[10] = &sys_anim_wait2,
	[11] = NULL,
	[12] = NULL,
	[13] = &sys_anim_wait3,
);
NODE(sys_anim_beyond, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_halt,
	[4] = &sys_anim_wait,
	[5] = &sys_anim_stop_all,
	[6] = &sys_anim_halt_all,
	[7] = &sys_anim_reset_all,
	[8] = &sys_anim_unpause_all,
	[9] = &sys_anim_init_alt,
	[10] = &sys_anim_init_from,
	[11] = NULL,
	[12] = &sys_anim_stream_running,
	[13] = &sys_anim_running,
);
NODE(sys_anim_kakyuusei, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_halt,
	[4] = &sys_anim_wait,
	[5] = &sys_anim_stop_all,
	[6] = &sys_anim_halt_all,
	[7] = &sys_anim_reset_all,
	[8] = &sys_anim_wait_all,
);

NODE(sys_anim_shuusaku, Anim,
	[0] = &sys_anim_init,
	[1] = &sys_anim_start,
	[2] = &sys_anim_stop,
	[3] = &sys_anim_start_sync,
	[4] = &sys_anim_halt,
	// 5 unused
	[6] = &sys_anim_unpause_all,
	[7] = &sys_anim_stop_all,
	// 8-15 unused
	[16] = &sys_anim_init2,
	[17] = &sys_anim_start2,
	[18] = &sys_anim_stop2,
	[19] = &sys_anim_start_sync2,
	[20] = &sys_anim_halt2,
);

// System.SaveData
LEAF(sys_savedata, resume_load);
LEAF(sys_savedata, resume_save);
LEAF(sys_savedata, load);
LEAF(sys_savedata, save);
LEAF(sys_savedata, load_var4);
LEAF(sys_savedata, save_var4);
LEAF(sys_savedata, save_union_var4);
LEAF(sys_savedata, load_var4_slice);
LEAF(sys_savedata, save_var4_slice);
LEAF(sys_savedata, copy);
LEAF(sys_savedata, set_mes_name);
LEAF(sys_savedata, clear_var4);
LEAF(sys_savedata, load_heap);
LEAF(sys_savedata, save_heap);
LEAF(sys_savedata, load_variables);
NODE(sys_savedata_classics, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = &sys_savedata_load,
	[3] = &sys_savedata_save,
	[4] = &sys_savedata_load_var4,
	[5] = &sys_savedata_save_var4,
	[6] = &sys_savedata_save_union_var4,
	[7] = &sys_savedata_load_var4_slice,
	[8] = &sys_savedata_save_var4_slice,
	[9] = &sys_savedata_copy,
	[13] = &sys_savedata_set_mes_name,
);
NODE(sys_savedata_isaku, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = &sys_savedata_load,
	[3] = &sys_savedata_save_union_var4,
	[4] = NULL, // load 50 dwords @ &System.var32[11]
	[5] = NULL, // save 50 dwords @ &System.var32[11]
	[6] = &sys_savedata_clear_var4,
);
NODE(sys_savedata_ai_shimai, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = &sys_savedata_load_var4,
	[3] = &sys_savedata_save_union_var4,
	[4] = NULL, // load 50 dwords @ &System.var32[11]
	[5] = NULL, // save 50 dwords @ &System.var32[11]
	[6] = &sys_savedata_clear_var4,
	[7] = &sys_savedata_load_heap,
	[8] = &sys_savedata_save_heap,
);
NODE(sys_savedata_allstars, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = NULL, // load var4 + 50 dwords @ &System.var32[12]
	[3] = &sys_savedata_save_union_var4,
	[4] = NULL, // load 50 dwords @ &System.var32[12]
	[5] = NULL, // save 50 dwords @ &System.var32[12]
	[6] = &sys_savedata_clear_var4,
	[7] = &sys_savedata_load_heap,
	[8] = &sys_savedata_save_heap,
	[9] = NULL, // in memory: save var4 + 50 dwords @ &System.var32[12]
	[10] = NULL, // in memory: restore saved var4 + 50 dwords @ &System.var32[12]
);
NODE(sys_savedata_doukyuusei, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = &sys_savedata_load_var4,
	[3] = &sys_savedata_save_union_var4,
	[4] = NULL, // load 200 dwords @ &System.var32[11]
	[5] = NULL, // save 200 dwords @ &System.var32[11]
	[6] = &sys_savedata_clear_var4,
	[7] = &sys_savedata_load_variables,
	[8] = NULL, // conditionally load some var4s at var[2001] - var4[3499]
	[9] = NULL, // conditionally save some var4s at var[2001] - var4[3499]
	[10] = &sys_savedata_save_var4,
);
NODE(sys_savedata_beyond, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = &sys_savedata_load_var4,
	[3] = &sys_savedata_save_union_var4,
	[4] = NULL, // load 200 dwords @ &System.var32[11]
	[5] = NULL, // save 200 dwords @ &System.var32[11]
	[6] = &sys_savedata_clear_var4,
	[7] = &sys_savedata_load_heap,
	[8] = &sys_savedata_save_heap,
);
NODE(sys_savedata_kakyuusei, SaveData,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_resume_save,
	[2] = &sys_savedata_load,
	[3] = &sys_savedata_save,
	[4] = &sys_savedata_load_variables,
);

LEAF(sys_savedata, load_var4_and_heap);
LEAF(sys_savedata, load_var32);
LEAF(sys_savedata, load_var16);
LEAF(sys_savedata, save_var16);
LEAF(sys_savedata, update_var4);

NODE(sys_load_shuusaku, Load,
	[0] = &sys_savedata_resume_load,
	[1] = &sys_savedata_load,
	[2] = &sys_savedata_load_var4_and_heap,
	[3] = &sys_savedata_load_var32,
	[4] = &sys_savedata_load_var16,
);
NODE(sys_save_shuusaku, Save,
	[0] = &sys_savedata_resume_save,
	[1] = &sys_savedata_save,
	[2] = &sys_savedata_save_var4,
	[3] = &sys_savedata_update_var4,
	[4] = &sys_savedata_save_var16,
);

// System.Audio
LEAF(sys_audio, bgm_play);
LEAF(sys_audio, bgm_play_sync);
LEAF(sys_audio, bgm_stop);
LEAF(sys_audio, bgm_set_volume);
LEAF(sys_audio, bgm_fade);
LEAF(sys_audio, bgm_fade_sync);
LEAF(sys_audio, bgm_fade_out);
LEAF(sys_audio, bgm_fade_out_sync);
LEAF(sys_audio, bgm_restore);
LEAF(sys_audio, se_play);
LEAF(sys_audio, se_stop);
LEAF(sys_audio, se_fade_out);
LEAF(sys_audio, se_fade_out_sync);
LEAF(sys_audio, se_play_sync);
LEAF(sys_audio, se_wait);
LEAF(sys_audio, bgm_set_next);
LEAF(sys_audio, bgm_play_next);
LEAF(sys_audio, aux_play);
LEAF(sys_audio, aux_stop);
LEAF(sys_audio, aux_fade_out);
LEAF(sys_audio, aux_fade_out_sync);
LEAF(sys_audio, bgm_is_playing);
NODE(sys_audio_classics, Audio,
	[0] = &sys_audio_bgm_play,
	[2] = &sys_audio_bgm_stop,
	[3] = &sys_audio_se_play,
	[4] = &sys_audio_bgm_fade_sync,
	[5] = &sys_audio_bgm_set_volume,
	[7] = &sys_audio_bgm_fade,
	[9] = &sys_audio_bgm_fade_out_sync,
	[10] = &sys_audio_bgm_fade_out,
	[12] = &sys_audio_se_stop,
	[18] = &sys_audio_bgm_restore,
);
NODE(sys_audio_isaku, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_fade_out,
	[2] = &sys_audio_bgm_stop,
	[3] = &sys_audio_se_play,
	[4] = &sys_audio_se_stop,
	[5] = &sys_audio_se_fade_out,
	[6] = &sys_audio_bgm_play_sync,
	[7] = &sys_audio_bgm_fade_out_sync,
	[8] = &sys_audio_se_fade_out_sync,
	[9] = &sys_audio_se_play_sync,
);
NODE(sys_audio_ai_shimai, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_stop,
	[2] = &sys_audio_bgm_fade_out,
	[3] = &sys_audio_bgm_fade_out_sync,
	[4] = &sys_audio_bgm_set_next,
	[5] = &sys_audio_bgm_play_next,
	// channel number (0-2) is parameter for these functions
	[6] = &sys_audio_aux_play,
	[7] = &sys_audio_aux_stop,
	[8] = &sys_audio_aux_fade_out,
	[9] = &sys_audio_aux_fade_out_sync,
);
NODE(sys_audio_allstars, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_stop,
	[2] = &sys_audio_bgm_fade_out,
	[3] = &sys_audio_bgm_fade_out_sync,
	[4] = &sys_audio_aux_play,
	[5] = &sys_audio_aux_stop,
	[6] = &sys_audio_aux_fade_out,
	[7] = &sys_audio_aux_fade_out_sync,
	[8] = &sys_audio_bgm_set_volume,
	[9] = &sys_audio_bgm_restore,
	[10] = &sys_audio_bgm_is_playing,
);
NODE(sys_audio_doukyuusei, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_fade_out,
	[2] = &sys_audio_bgm_stop,
	[3] = &sys_audio_se_play,
	[4] = &sys_audio_se_stop,
	[5] = &sys_audio_se_fade_out,
	[6] = &sys_audio_bgm_play_sync,
	[7] = &sys_audio_se_play_sync, // with cancel
);
NODE(sys_audio_beyond, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_stop,
	[2] = &sys_audio_bgm_fade_out,
	[3] = &sys_audio_bgm_fade_out_sync,
	[4] = &sys_audio_se_play,
	[5] = &sys_audio_se_stop,
	[6] = &sys_audio_se_fade_out,
	[7] = &sys_audio_se_fade_out_sync,
	[8] = &sys_audio_bgm_set_volume,
	[9] = &sys_audio_bgm_restore,
	[10] = &sys_audio_bgm_is_playing,
);
NODE(sys_audio_kakyuusei, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_stop,
	[2] = &sys_audio_bgm_fade_out,
	[3] = &sys_audio_se_play,
	[4] = &sys_audio_se_fade_out,
	[5] = &sys_audio_se_stop,
);
NODE(sys_audio_doukyuusei2, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_fade_out,
	[2] = &sys_audio_bgm_stop,
	[3] = &sys_audio_se_play,
	[4] = &sys_audio_se_stop,
);

NODE(sys_audio_shuusaku, Audio,
	[0] = &sys_audio_bgm_play,
	[1] = &sys_audio_bgm_stop,
	[3] = &sys_audio_bgm_fade_out,
	[5] = &sys_audio_bgm_set_volume,
	[16] = &sys_audio_se_play,
	[17] = &sys_audio_se_stop,
	[19] = &sys_audio_se_fade_out,
	[20] = &sys_audio_se_fade_out_sync,
	[22] = &sys_audio_se_wait,
);

// System.Voice
LEAF(sys_voice, play);
LEAF(sys_voice, stop);
LEAF(sys_voice, play_sync);
LEAF(sys_voice, prepare);
LEAF(sys_voice, play_prepared);
LEAF(sys_voice, play_prepared_sync);
LEAF(sys_voice, is_playing);
LEAF(sys_voice, set_volume);
LEAF(sys_voice, restore_volume);
NODE(sys_voice, Voice,
	[0] = &sys_voice_play,
	[1] = &sys_voice_stop,
	[2] = &sys_voice_play_sync,
);
NODE(sys_voice_ai_shimai, Voice,
	[0] = &sys_voice_play,
	[1] = &sys_voice_stop,
	[2] = &sys_voice_play_sync,
	[3] = &sys_voice_prepare,
	[4] = &sys_voice_play_prepared,
	[5] = &sys_voice_is_playing,
);
NODE(sys_voice_allstars, Voice,
	[0] = &sys_voice_play,
	[1] = &sys_voice_stop,
	[2] = &sys_voice_play_sync,
	[3] = &sys_voice_is_playing,
	[4] = &sys_voice_set_volume,
	[5] = &sys_voice_restore_volume,
);
NODE(sys_voice_doukyuusei, Voice,
	[0] = &sys_voice_play,
	[1] = &sys_voice_stop,
	[2] = &sys_voice_play_sync,
	[3] = &sys_voice_is_playing,
);
NODE(sys_voice_beyond, Voice,
	[0] = &sys_voice_play,
	[1] = &sys_voice_stop,
	[2] = &sys_voice_play_sync,
	[3] = &sys_voice_is_playing,
	[4] = &sys_voice_set_volume,
	[5] = &sys_voice_restore_volume,
	[6] = &sys_voice_prepare,
	[7] = &sys_voice_play_prepared,
	[8] = &sys_voice_play_prepared_sync,
);
NODE(sys_voice_doukyuusei2, Voice,
	[0] = &sys_voice_play,
	[1] = &sys_voice_stop,
	[2] = &sys_voice_play_sync,
);
NODE(sys_voice_kakyuusei, Voice,
	[0] = &sys_voice_play,
	[1] = &sys_voice_stop,
	[2] = &sys_voice_is_playing,
);

// System.File
LEAF(sys_file, read);
LEAF(sys_file, write);
NODE(sys_file, File,
	[0] = &sys_file_read,
	[1] = &sys_file_write,
);

// System.load_file
LEAF(sys, load_file);

// System.load_image
LEAF(sys, load_image);

// System.Palette
LEAF(sys_palette, set);
LEAF(sys_palette, crossfade);
LEAF(sys_palette, crossfade_timed);
LEAF(sys_palette, crossfade_range);
LEAF(sys_palette, hide);
LEAF(sys_palette, unhide);
LEAF(sys_palette, update);
NODE(sys_palette, Palette,
	[0] = &sys_palette_set,
	[1] = &sys_palette_crossfade,
	[2] = &sys_palette_crossfade_timed,
	[3] = &sys_palette_hide,
	[4] = &sys_palette_unhide,
);

NODE(sys_palette_kakyuusei, Palette,
	[0] = NULL, // unused
	[1] = &sys_palette_crossfade,
	[2] = NULL, // unused
	[3] = NULL, // unused
	[4] = NULL, // unused
	[5] = &sys_palette_update,
	[6] = NULL, // unused
	[7] = &sys_palette_crossfade_range,
);

// System.Display
LEAF2(sys_display, freeze, unfreeze);
LEAF2(sys_display, fade_out, fade_in);
LEAF2(sys_display, scan_out, scan_in);
LEAF2(sys_display, hide, unhide);
NODE(sys_display, Display,
	[0] = &sys_display_freeze_unfreeze,
	[1] = &sys_display_fade_out_fade_in,
	[2] = &sys_display_scan_out_scan_in,
);
NODE(sys_display_ai_shimai, Display,
	[0] = &sys_display_hide_unhide,
	[1] = &sys_display_fade_out_fade_in,
);
NODE(sys_display_allstars, Display,
	[0] = &sys_display_hide_unhide,
	[1] = &sys_display_fade_out_fade_in,
);
NODE(sys_display_doukyuusei, Display,
	[0] = &sys_display_hide_unhide,
	[1] = &sys_display_fade_out_fade_in,
);
NODE(sys_display_beyond, Display,
	[0] = &sys_display_hide_unhide,
	[1] = &sys_display_fade_out_fade_in,
);

// System.Image
LEAF(sys_image, copy);
LEAF(sys_image, copy_masked);
LEAF(sys_image, fill_bg);
LEAF(sys_image, copy_swap);
LEAF(sys_image, swap_bg_fg);
LEAF(sys_image, compose);
LEAF(sys_image, invert_colors);
LEAF(sys_image, copy_progressive);
LEAF(sys_image, blend);
LEAF(sys_image, blend_masked);
LEAF(sys_image, blend_to);
LEAF(sys_image, pixel_fade);
LEAF(sys_image, pixel_fade_slow);
LEAF(sys_image, pixel_fade_masked);
LEAF(sys_image, blend_half);
LEAF(sys_image, blend_with_mask_color);
LEAF(sys_image, darken);
LEAF(sys_image, crossfade);
NODE(sys_image_classics, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = &sys_image_copy_swap,
	[4] = &sys_image_swap_bg_fg,
	[5] = &sys_image_compose,
	[6] = &sys_image_invert_colors,
	[20] = &sys_image_copy_progressive,
);
NODE(sys_image_isaku, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = &sys_image_copy_swap,
	[4] = &sys_image_swap_bg_fg,
	[5] = &sys_image_copy_progressive,
	[6] = &sys_image_compose,
);
NODE(sys_image_ai_shimai, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = NULL, // unused
	[4] = &sys_image_swap_bg_fg,
	[5] = NULL, // unused
	[6] = &sys_image_blend,
	[7] = &sys_image_blend_masked,
);
NODE(sys_image_allstars, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = &sys_image_copy_swap,
	[4] = NULL, // unused
	[5] = &sys_image_pixel_fade,
	[6] = NULL, // unused
	[7] = &sys_image_blend,
	[8] = &sys_image_blend_half,
	[9] = NULL, // unused
	[10] = &sys_image_blend_with_mask_color,
);
NODE(sys_image_doukyuusei, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = &sys_image_copy_swap,
	[4] = &sys_image_swap_bg_fg,
	[5] = &sys_image_pixel_fade,
	[6] = &sys_image_compose,
	[7] = NULL, // unused
	[8] = &sys_image_invert_colors,
	[9] = &sys_image_pixel_fade_masked,
	[10] = NULL, // unused
	[11] = &sys_image_darken,
	[12] = NULL, // unused
	[13] = NULL, // unused
	[14] = &sys_image_blend_to,
);
NODE(sys_image_beyond, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = &sys_image_copy_swap,
	[4] = &sys_image_swap_bg_fg,
	[5] = NULL, // TODO
	[6] = &sys_image_compose,
	[7] = NULL, // TODO
	[8] = NULL, // TODO
	[9] = NULL, // TODO
	[10] = &sys_image_blend_with_mask_color,
	[11] = &sys_image_crossfade,
	[12] = NULL, // TODO
);
NODE(sys_image_kakyuusei, Image,
	[0] = &sys_image_copy,
	[1] = &sys_image_copy_masked,
	[2] = &sys_image_fill_bg,
	[3] = &sys_image_copy_swap,
	[4] = &sys_image_swap_bg_fg,
	[5] = &sys_image_pixel_fade,
	[6] = &sys_image_compose,
	[7] = &sys_image_pixel_fade_slow,
);

// System.wait
LEAF(sys, wait);

// System.set_text_colors
LEAF(sys, set_text_colors);

// System.farcall
LEAF(sys, farcall);

// System.get_cursor_segment
LEAF(sys, get_cursor_segment);

// System.get_menu_no
LEAF(sys, get_menu_no);

// System.get_time
LEAF(sys, get_time);

// System.Map
LEAF(sys_map, load_tilemap);
LEAF(sys_map, load_sprite);
LEAF(sys_map, load_tiles);
LEAF(sys_map, load_sprite_scripts);
LEAF(sys_map, set_sprite_script);
LEAF(sys_map, place_sprites);
LEAF(sys_map, set_sprite_state);
LEAF(sys_map, tick_and_redraw);
LEAF(sys_map, tick);
LEAF(sys_map, draw_tiles);
LEAF(sys_map, draw_tiles2);
LEAF(sys_map, set_location_mode);
LEAF(sys_map, get_location);
LEAF(sys_map, move_sprite);
LEAF(sys_map, path_sprite);
LEAF(sys_map, cancel_sprite_pathing);
LEAF(sys_map, get_pathing);
LEAF(sys_map, rewind_sprite_pos);
LEAF(sys_map, load_palette);
LEAF(sys_map, load_bitmap);
NODE(sys_map_allstars, Map,
	[0] = &sys_map_load_tilemap,
	[1] = &sys_map_load_sprite,
	[2] = &sys_map_load_tiles,
	[3] = NULL, // unused?
	[4] = &sys_map_load_sprite_scripts,
	[5] = &sys_map_set_sprite_script,
	[6] = &sys_map_place_sprites,
	[7] = &sys_map_set_sprite_state,
	[8] = &sys_map_tick_and_redraw,
	[9] = &sys_map_tick,
	[10] = &sys_map_draw_tiles,
	[11] = &sys_map_draw_tiles2,
	[12] = &sys_map_set_location_mode,
	[13] = &sys_map_get_location,
	[14] = &sys_map_move_sprite,
	[15] = &sys_map_path_sprite,
	[16] = &sys_map_cancel_sprite_pathing,
	[17] = NULL, // unused?
	[18] = NULL, // unused?
	[19] = NULL, // unused?
	[20] = &sys_map_rewind_sprite_pos,
	[21] = NULL, // unused?
	[22] = NULL, // unused?
	[23] = NULL, // unused?
	[24] = &sys_map_load_palette,
	[25] = &sys_map_load_bitmap,
);
NODE(sys_map_doukyuusei, Map,
	[0] = &sys_map_load_tilemap,
	[1] = &sys_map_load_sprite,
	[2] = &sys_map_load_tiles,
	[3] = NULL, // unused?
	[4] = &sys_map_load_sprite_scripts,
	[5] = &sys_map_set_sprite_script,
	[6] = &sys_map_place_sprites,
	[7] = &sys_map_set_sprite_state,
	[8] = &sys_map_tick_and_redraw,
	[9] = &sys_map_tick,
	[10] = &sys_map_draw_tiles,
	[11] = &sys_map_draw_tiles2,
	[12] = &sys_map_set_location_mode,
	[13] = &sys_map_get_location,
	[14] = &sys_map_move_sprite,
	[15] = &sys_map_path_sprite,
	[16] = &sys_map_cancel_sprite_pathing,
	[17] = &sys_map_get_pathing,
	[18] = NULL, // unused?
	[19] = NULL, // unused?
	[20] = &sys_map_rewind_sprite_pos,
	[21] = NULL, // unused?
	[22] = NULL, // TODO
	[23] = NULL, // unused?
	[24] = &sys_map_load_palette,
	[25] = &sys_map_load_bitmap,
);
NODE(sys_map_kakyuusei, Map,
	[0] = &sys_map_load_tilemap,
	[1] = &sys_map_load_sprite,
	[2] = &sys_map_load_tiles,
	[3] = NULL, // unused?
	[4] = &sys_map_load_sprite_scripts,
	[5] = &sys_map_set_sprite_script,
	[6] = &sys_map_place_sprites,
	[7] = &sys_map_set_sprite_state,
	[8] = &sys_map_tick_and_redraw,
	[9] = &sys_map_tick,
	[10] = &sys_map_draw_tiles,
	[11] = &sys_map_draw_tiles2,
	[12] = &sys_map_set_location_mode,
	[13] = &sys_map_get_location,
	[14] = &sys_map_move_sprite,
	[15] = &sys_map_path_sprite,
	[16] = &sys_map_cancel_sprite_pathing,
	[17] = &sys_map_get_pathing,
	[18] = NULL, // unused?
	[19] = NULL, // unused?
	[20] = &sys_map_rewind_sprite_pos,
	[21] = NULL, // TODO
);

// System.noop
LEAF(sys, noop);

// System.Dungeon
NODE(sys_dungeon, Dungeon,
);

// System.input_state
LEAF(sys, check_input);

// System.Backlog
LEAF(sys_backlog, clear);
LEAF(sys_backlog, prepare);
LEAF(sys_backlog, commit);
LEAF(sys_backlog, get_count);
LEAF(sys_backlog, get_pointer);
LEAF(sys_backlog, has_voice);
NODE(sys_backlog_aishimai, Backlog,
	[0] = &sys_backlog_clear,
	[1] = &sys_backlog_prepare,
	[2] = &sys_backlog_commit,
	[3] = &sys_backlog_get_count,
	[4] = &sys_backlog_get_pointer,
	[5] = &sys_backlog_has_voice,
);
NODE(sys_backlog_doukyuusei, Backlog,
	[0] = &sys_backlog_clear,
	[1] = &sys_backlog_prepare,
	[2] = &sys_backlog_commit,
	[3] = &sys_backlog_get_count,
	[4] = &sys_backlog_get_pointer,
);
NODE(sys_backlog_beyond, Backlog,
	[0] = &sys_backlog_clear,
	[1] = &sys_backlog_prepare,
	[2] = &sys_backlog_commit,
	[3] = &sys_backlog_get_count,
	[4] = &sys_backlog_get_pointer,
	[5] = &sys_backlog_has_voice,
);
NODE(sys_backlog_kakyuusei, Backlog,
	[0] = &sys_backlog_clear,
	[1] = &sys_backlog_prepare,
	[2] = &sys_backlog_commit,
	[3] = &sys_backlog_get_count,
	[4] = &sys_backlog_get_pointer,
);

// System.noop2
LEAF(sys, noop2);

// System.strlen
LEAF(sys, strlen);

// System.set_screen_surface
LEAF(sys, set_screen_surface);

// System.ItemWindow
LEAF(sys_itemwindow, init);
LEAF(sys_itemwindow, open);
LEAF(sys_itemwindow, is_open);
LEAF(sys_itemwindow, get_pos);
LEAF(sys_itemwindow, get_cursor_pos);
LEAF(sys_itemwindow, enable);
LEAF(sys_itemwindow, disable);
LEAF(sys_itemwindow, update);
NODE(sys_itemwindow, ItemWindow,
	[0] = &sys_itemwindow_init,
	[1] = &sys_itemwindow_open,
	[2] = &sys_itemwindow_is_open,
	[3] = &sys_itemwindow_get_pos,
	[4] = &sys_itemwindow_get_cursor_pos,
	[5] = &sys_itemwindow_enable,
	[6] = &sys_itemwindow_disable,
	[7] = NULL,
	[8] = &sys_itemwindow_update,
	[9] = NULL,
	[10] = NULL,
);

// System.SaveMenu
LEAF(sys_savemenu, open);
LEAF(sys_savemenu, enable);
LEAF(sys_savemenu, clear);
LEAF(sys_savemenu, check);
NODE(sys_savemenu, SaveMenu,
	[0] = &sys_savemenu_open,
	[1] = &sys_savemenu_enable,
	[2] = &sys_savemenu_clear,
	[3] = &sys_savemenu_check,
);

// System.LoadMenu
LEAF(sys_loadmenu, open);
LEAF(sys_loadmenu, enable);
LEAF(sys_loadmenu, clear);
LEAF(sys_loadmenu, check);
NODE(sys_loadmenu, LoadMenu,
	[0] = &sys_loadmenu_open,
	[1] = &sys_loadmenu_enable,
	[2] = &sys_loadmenu_clear,
	[3] = &sys_loadmenu_check,
);

// System.Message
LEAF(sys_msg, enable_clear);
LEAF(sys_msg, disable_clear);
LEAF(sys_msg, clear);
NODE(sys_msg, Message,
	[0] = &sys_msg_enable_clear,
	[1] = &sys_msg_disable_clear,
	[2] = &sys_msg_clear,
);

// System.Overlay
LEAF(sys_overlay, update_text);
LEAF(sys_overlay, clear_text);
NODE(sys_overlay, Overlay,
	[0] = NULL,
	[1] = &sys_overlay_update_text,
	[2] = &sys_overlay_clear_text,
	[3] = NULL,
);

// System.IME
LEAF(sys_ime, enable);
LEAF(sys_ime, disable);
LEAF(sys_ime, get_composition_started);
LEAF(sys_ime, get_text);
LEAF(sys_ime, get_cursor_inside);
LEAF(sys_ime, get_cursor_pos);
LEAF(sys_ime, strcmp);
LEAF(sys_ime, get_composition_state);
NODE(sys_ime, IME,
	[0] = &sys_ime_enable,
	[1] = &sys_ime_disable,
	[2] = &sys_ime_get_composition_started,
	[3] = &sys_ime_get_text,
	[4] = &sys_ime_get_cursor_inside,
	[5] = &sys_ime_get_cursor_pos,
	[6] = &sys_ime_strcmp,
	[7] = &sys_ime_get_composition_state,
);

// System.FaceWindow (allstars)
NODE(sys_face_window, FaceWindow,
	[0] = NULL, // unused?
	[1] = NULL, // unused?
	[2] = NULL,
);

// System.run_mahjong (allstars)
LEAF(sys, run_mahjong);

// System.check_disc (doukyuusei 2)
LEAF(sys, check_disc);

// System.wait_click (doukyuusei 2)
LEAF(sys, wait_click);

PUBLIC_NODE(mes_sys_classics, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_classics,
	[3] = &sys_anim,
	[4] = &sys_savedata_classics,
	[5] = &sys_audio_classics,
	[7] = &sys_file,
	[8] = &sys_load_image,
	[9] = &sys_palette,
	[10] = &sys_image_classics,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_noop,
	[18] = &sys_check_input,
	[20] = &sys_noop2,
	[21] = &sys_strlen,
	[23] = &sys_set_screen_surface,
);

PUBLIC_NODE(mes_sys_isaku, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_isaku,
	[3] = &sys_anim,
	[4] = &sys_savedata_isaku,
	[5] = &sys_audio_isaku,
	[6] = &sys_voice,
	[7] = &sys_load_file,
	[8] = &sys_load_image,
	[9] = &sys_display,
	[10] = &sys_image_isaku,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_noop,
	[18] = &sys_check_input,
	[20] = &sys_dungeon,
	[22] = &sys_itemwindow,
	[24] = &sys_strlen,
	[25] = &sys_savemenu,
	[26] = &sys_loadmenu,
	[27] = &sys_msg,
);

PUBLIC_NODE(mes_sys_ai_shimai, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_ai_shimai,
	[3] = &sys_anim_ai_shimai,
	[4] = &sys_savedata_ai_shimai,
	[5] = &sys_audio_ai_shimai,
	[6] = &sys_voice_ai_shimai,
	[7] = &sys_file,
	[8] = &sys_load_image,
	[9] = &sys_display_ai_shimai,
	[10] = &sys_image_ai_shimai,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_noop,
	[18] = &sys_check_input,
	[19] = &sys_backlog_aishimai,
	[20] = &sys_noop2,
	[21] = &sys_strlen,
	[22] = &sys_overlay,
	[23] = &sys_ime,
);

PUBLIC_NODE(mes_sys_allstars, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_allstars,
	[3] = &sys_anim_allstars,
	[4] = &sys_savedata_allstars,
	[5] = &sys_audio_allstars,
	[6] = &sys_voice_allstars,
	[7] = &sys_load_file,
	[8] = &sys_load_image,
	[9] = &sys_display_allstars,
	[10] = &sys_image_allstars,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_map_allstars,
	[18] = &sys_check_input,
	[19] = NULL, // unused
	[20] = NULL, // unused
	[21] = &sys_strlen,
	//[22] = overlay?
	[23] = &sys_face_window,
	[24] = &sys_run_mahjong,
);

PUBLIC_NODE(mes_sys_doukyuusei, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_doukyuusei,
	[3] = &sys_anim_doukyuusei,
	[4] = &sys_savedata_doukyuusei,
	[5] = &sys_audio_doukyuusei,
	[6] = &sys_voice_doukyuusei,
	[7] = &sys_load_file,
	[8] = &sys_load_image,
	[9] = &sys_display_doukyuusei,
	[10] = &sys_image_doukyuusei,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_map_doukyuusei,
	[18] = &sys_check_input,
	[19] = &sys_backlog_doukyuusei,
	[20] = NULL, // unused
	[21] = NULL, // unused
	[22] = NULL, // unused
	[23] = NULL, // unused
	[24] = &sys_strlen,
	[25] = NULL, // TODO
	[26] = NULL, // TODO
);

PUBLIC_NODE(mes_sys_doukyuusei2, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_kakyuusei,
	[3] = &sys_anim_kakyuusei,
	[4] = &sys_savedata_kakyuusei,
	[5] = &sys_audio_doukyuusei2,
	[6] = &sys_voice_doukyuusei2,
	[7] = &sys_load_file,
	[8] = &sys_load_image,
	[9] = &sys_palette_kakyuusei,
	[10] = &sys_image_kakyuusei,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_map_kakyuusei,
	[18] = &sys_backlog_kakyuusei,
	[19] = &sys_check_disc,
	[20] = &sys_wait_click,
);

PUBLIC_NODE(mes_sys_beyond, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_beyond,
	[3] = &sys_anim_beyond,
	[4] = &sys_savedata_beyond,
	[5] = &sys_audio_beyond,
	[6] = &sys_voice_beyond,
	[7] = &sys_load_file,
	[8] = &sys_load_image,
	[9] = &sys_display_beyond,
	[10] = &sys_image_beyond,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = NULL,
	[18] = &sys_check_input,
	[19] = &sys_backlog_beyond,
	[21] = &sys_strlen,
	//[22] = overlay?
	[23] = NULL, // TODO
);

PUBLIC_NODE(mes_sys_kakyuusei, System,
	[0] = &sys_set_font_size,
	[1] = &sys_display_number,
	[2] = &sys_cursor_kakyuusei,
	[3] = &sys_anim_kakyuusei,
	[4] = &sys_savedata_kakyuusei,
	[5] = &sys_audio_kakyuusei,
	[6] = &sys_voice_kakyuusei,
	[7] = &sys_load_file,
	[8] = &sys_load_image,
	[9] = &sys_palette_kakyuusei,
	[10] = &sys_image_kakyuusei,
	[11] = &sys_wait,
	[12] = &sys_set_text_colors,
	[13] = &sys_farcall,
	[14] = &sys_get_cursor_segment,
	[15] = &sys_get_menu_no,
	[16] = &sys_get_time,
	[17] = &sys_map_kakyuusei,
	[18] = &sys_backlog_kakyuusei,
);

LEAF(util_plan, fini);
LEAF(util_plan, init);
LEAF(util_plan, push);
LEAF(util_plan, pop);
NODE(util_plan, Plan,
	[0] = &util_plan_fini,
	[1] = &util_plan_init,
	[2] = &util_plan_push,
	[3] = &util_plan_pop,
);

LEAF(util_message_window, hide);
LEAF(util_message_window, show);
NODE(util_message_window, MessageWindow,
	[0] = &util_message_window_hide,
	[1] = &util_message_window_show,
);

LEAF(util_credits, header);
LEAF(util_credits, eri);
LEAF(util_credits, player);
NODE(util_credits, Credits,
	[0] = &util_credits_header,
	[1] = &util_credits_eri,
	[2] = &util_credits_player,
);

LEAF(util, pixel_crossfade);
LEAF(util, update_schedule);
LEAF(util, load_stored_palette);
LEAF(util, photo_slide);
LEAF(util, status_dirty);
LEAF(util, pixel_palette_crossfade);
LEAF(util, draw_clock);
LEAF(util, zoom_movie);
LEAF(util, zoom_cam);
LEAF(util, crossfade_high_colors);
LEAF(util, eri_pixel_crossfade);
LEAF(util, scene_viewer_char_select);
LEAF(util, scene_viewer_scene_select);
LEAF(util, scene_viewer_zoom_out);
LEAF(util, name_input);
LEAF(util, draw_myouji);
LEAF(util, draw_namae);
LEAF(util, scroll_down);
LEAF(util, pixel_melt);
LEAF(util, anim_wait);
LEAF(util, set_screen_y);
LEAF(util, quake);
LEAF(util, ending_logo_crossfade);
LEAF(util, pixel_crossfade_low);
LEAF(util, set_config_enabled);
NODE(sys_util_shuusaku, Util,
	[0] = &util_pixel_crossfade,
	[1] = &util_plan,
	[2] = &util_message_window,
	[3] = &util_update_schedule,
	[4] = NULL,
	[5] = &util_load_stored_palette,
	[6] = &util_photo_slide,
	[7] = &util_status_dirty,
	[8] = &util_pixel_palette_crossfade,
	[9] = &util_draw_clock,
	[10] = &util_zoom_movie,
	[11] = &util_zoom_cam,
	[12] = &util_crossfade_high_colors,
	[13] = &util_eri_pixel_crossfade,
	[14] = &util_scene_viewer_char_select,
	[15] = &util_scene_viewer_scene_select,
	[16] = NULL,
	[17] = &util_scene_viewer_zoom_out,
	[18] = &util_name_input,
	[19] = &util_draw_myouji,
	[20] = &util_draw_namae,
	[21] = &util_credits,
	[22] = &util_scroll_down,
	[23] = &util_pixel_melt,
	[24] = &util_anim_wait,
	[25] = &util_set_screen_y,
	[26] = &util_quake,
	[27] = &util_ending_logo_crossfade,
	[28] = NULL,
	[29] = &util_pixel_crossfade_low,
	[30] = &util_set_config_enabled,
);

LEAF(aiw_sys, display_number);
LEAF(aiw_sys, set_text_color);
LEAF(aiw_sys, wait);
LEAF(aiw_sys, text_clear);
LEAF(aiw_sys, commit_message);
LEAF(aiw_sys, load_image);
LEAF(aiw_sys, surface_copy);
LEAF(aiw_sys, surface_copy_masked);
LEAF(aiw_sys, surface_swap);
LEAF(aiw_sys, surface_fill);
LEAF(aiw_sys, surface_invert);
LEAF(aiw_sys, set_color);
LEAF2(aiw_sys, hide, show);
LEAF(aiw_sys, crossfade);
LEAF(aiw_sys, crossfade2);
LEAF(aiw_sys, Cursor); // TODO: not leaf
LEAF(aiw_sys, load_audio);
LEAF(aiw_sys, load_effect);
LEAF(aiw_sys, load_voice);
LEAF(aiw_sys, play_movie);
LEAF(aiw_sys, OP_0x34);

// XXX: AIWIN games are indexed by opcode
PUBLIC_NODE(mes_sys_shuusaku, System,
	[AIW_MES_STMT_UTIL] = &sys_util_shuusaku,
	[AIW_MES_STMT_LOAD] = &sys_load_shuusaku,
	[AIW_MES_STMT_SAVE] = &sys_save_shuusaku,
	[AIW_MES_STMT_NUM] = &aiw_sys_display_number,
	[AIW_MES_STMT_SET_TEXT_COLOR] = &aiw_sys_set_text_color,
	[AIW_MES_STMT_WAIT] = &aiw_sys_wait,
	[AIW_MES_STMT_21] = &aiw_sys_text_clear,
	[AIW_MES_STMT_COMMIT_MESSAGE] = &aiw_sys_commit_message,
	[AIW_MES_STMT_LOAD_IMAGE] = &aiw_sys_load_image,
	[AIW_MES_STMT_SURF_COPY] = &aiw_sys_surface_copy,
	[AIW_MES_STMT_SURF_COPY_MASKED] = &aiw_sys_surface_copy_masked,
	[AIW_MES_STMT_SURF_SWAP] = &aiw_sys_surface_swap,
	[AIW_MES_STMT_SURF_FILL] = &aiw_sys_surface_fill,
	[AIW_MES_STMT_SURF_INVERT] = &aiw_sys_surface_invert,
	[AIW_MES_STMT_29] = &aiw_sys_set_color,
	[AIW_MES_STMT_SHOW_HIDE] = &aiw_sys_hide_show,
	[AIW_MES_STMT_CROSSFADE] = &aiw_sys_crossfade,
	[AIW_MES_STMT_CROSSFADE2] = &aiw_sys_crossfade2,
	[AIW_MES_STMT_CURSOR] = &aiw_sys_Cursor,
	[AIW_MES_STMT_ANIM] = &sys_anim_shuusaku,
	[AIW_MES_STMT_LOAD_AUDIO] = &aiw_sys_load_audio,
	[AIW_MES_STMT_LOAD_EFFECT] = &aiw_sys_load_effect,
	[AIW_MES_STMT_LOAD_VOICE] = &aiw_sys_load_voice,
	[AIW_MES_STMT_AUDIO] = &sys_audio_shuusaku,
	[AIW_MES_STMT_PLAY_MOVIE] = &aiw_sys_play_movie,
	[AIW_MES_STMT_34] = &aiw_sys_OP_0x34,
);

PUBLIC_NODE(mes_sys_none, System,);

static bool component_name_equals(struct mes_path_component *c, const char *name)
{
	if (!strcmp(c->name, name))
		return true;
	if (c->name_noargs && !strcmp(c->name_noargs, name))
		return true;
	return false;
}

static struct mes_path_component *_resolve_qname(struct mes_path_component *ctx,
		struct mes_qname_part *part, int *no)
{
	if (part->type == MES_QNAME_NUMBER) {
		*no = part->number;
		if (*no < ctx->nr_children && ctx->children[*no])
			return ctx->children[*no];
		return NULL;
	}
	for (int i = 0; i < ctx->nr_children; i++) {
		if (!ctx->children[i])
			continue;
		if (component_name_equals(ctx->children[i], part->ident)) {
			*no = i;
			return ctx->children[i];
		}
	}
	*no = -1;
	return NULL;
}

mes_parameter_list mes_resolve_syscall(mes_qname name, int *no)
{
	mes_parameter_list params = vector_initializer;

	// get syscall number
	if (vector_length(name) < 1)
		goto error;

	struct mes_path_component *ctx = _resolve_qname(mes_code_tables.system, &kv_A(name, 0), no);
	if (*no < 0)
		goto error;

	// resolve remaining identifiers
	for (int i = 1; ctx && i < vector_length(name); i++) {
		int part_no;
		struct mes_qname_part *part = &kv_A(name, i);
		ctx = _resolve_qname(ctx, part, &part_no);
		if (part_no < 0)
			goto error;
		if (part->type == MES_QNAME_IDENT)
			string_free(part->ident);
		part->type = MES_QNAME_NUMBER;
		part->number = part_no;
	}

	// create parameter list from resolved identifiers
	for (int i = 1; i < vector_length(name); i++) {
		struct mes_qname_part *part = &kv_A(name, i);
		if (part->type == MES_QNAME_IDENT)
			goto error;
		struct mes_parameter param = { 
			.type = MES_PARAM_EXPRESSION,
			.expr = xcalloc(1, sizeof(struct mes_expression))
		};
		if (part->number > 127) {
			param.expr->op = MES_EXPR_IMM16;
			param.expr->arg16 = part->number;
		} else {
			param.expr->op = MES_EXPR_IMM;
			param.expr->arg8 = part->number;
		}
		vector_push(struct mes_parameter, params, param);
	}

	mes_qname_free(name);
	return params;
error:
	vector_destroy(params);
	*no = -1;
	return (mes_parameter_list)vector_initializer;
}

static struct mes_path_component *get_child(struct mes_path_component *parent, unsigned no)
{
	if (no >= parent->nr_children)
		return NULL;
	return parent->children[no];
}

static string get_name(string name, struct mes_path_component *parent,
		mes_parameter_list params, unsigned *skip_params)
{
	if (vector_length(params) <= *skip_params)
		return name;
	if (parent->nr_children == 0 || vector_A(params, *skip_params).type != MES_PARAM_EXPRESSION)
		return name;
	struct mes_expression *expr = vector_A(params, *skip_params).expr;
	unsigned no = 0;
	if (expr->op == MES_EXPR_IMM)
		no = expr->arg8;
	else if (expr->op == MES_EXPR_IMM16)
		no = expr->arg16;
	else if (expr->op == MES_EXPR_IMM32)
		no = expr->arg32;
	else
		return name;

	(*skip_params)++;

	struct mes_path_component *child = get_child(parent, no);
	if (!child) {
		return string_concat_fmt(name, ".function[%u]", no);
	}

	if (*skip_params == vector_length(params) && child->name_noargs) {
		name = string_concat_fmt(name, ".%s", child->name_noargs);
	} else {
		name = string_concat_fmt(name, ".%s", child->name);
	}
	return get_name(name, child, params, skip_params);
}

string mes_get_syscall_name(unsigned no, mes_parameter_list params, unsigned *skip_params,
		const char *ns)
{
	*skip_params = 0;
	string name = string_empty();
	if (ns)
		name = string_concat_fmt(name, "%s.", ns);

	struct mes_path_component *sys = get_child(mes_code_tables.system, no);
	if (!sys) {
		return string_concat_fmt(name, "function[%u]", no);
	}

	name = string_concat_fmt(name, "%s", sys->name);
	return get_name(name, sys, params, skip_params);
}

const char *mes_system_var16_names[MES_NR_SYSTEM_VARIABLES] = {
	[MES_SYS_VAR_HEAP] = "heap",
	[MES_SYS_VAR_DST_SURFACE] = "dst_surface",
	[MES_SYS_VAR_FLAGS] = "flags",
	[MES_SYS_VAR_CURSOR_X] = "cursor_x",
	[MES_SYS_VAR_CURSOR_Y] = "cursor_y",
	[MES_SYS_VAR_TEXT_START_X] = "text_start_x",
	[MES_SYS_VAR_TEXT_START_Y] = "text_start_y",
	[MES_SYS_VAR_TEXT_END_X] = "text_end_x",
	[MES_SYS_VAR_TEXT_END_Y] = "text_end_y",
	[MES_SYS_VAR_TEXT_CURSOR_X] = "text_cursor_x",
	[MES_SYS_VAR_TEXT_CURSOR_Y] = "text_cursor_y",
	[MES_SYS_VAR_BG_COLOR] = "bg_color",
	[MES_SYS_VAR_FG_COLOR] = "fg_color",
	[MES_SYS_VAR_DISPLAY_NUMBER_FLAGS] = "display_number_flags",
	[MES_SYS_VAR_FONT_WIDTH] = "font_width",
	[MES_SYS_VAR_FONT_HEIGHT] = "font_height",
	[MES_SYS_VAR_FONT_WEIGHT] = "font_weight",
	[MES_SYS_VAR_CHAR_SPACE] = "char_space",
	[MES_SYS_VAR_LINE_SPACE] = "line_space",
	[MES_SYS_VAR_CG_X] = "cg_x",
	[MES_SYS_VAR_CG_Y] = "cg_y",
	[MES_SYS_VAR_CG_W] = "cg_w",
	[MES_SYS_VAR_CG_H] = "cg_h",
	[MES_SYS_VAR_NR_MENU_ENTRIES] = "nr_menu_entries",
	[MES_SYS_VAR_MENU_NO] = "menu_no",
	[MES_SYS_VAR_MASK_COLOR] = "mask_color",
};

const char *mes_system_var32_names[MES_NR_SYSTEM_VARIABLES] = {
	[MES_SYS_VAR_MEMORY] = "memory",
	[MES_SYS_VAR_CG_OFFSET] = "cg_offset",
	[MES_SYS_VAR_DATA_OFFSET] = "data_offset",
	[MES_SYS_VAR_MPX_OFFSET] = "mpx_offset",
	[MES_SYS_VAR_CCD_OFFSET] = "ccd_offset",
	[MES_SYS_VAR_EVE_OFFSET] = "eve_offset",
	[MES_SYS_VAR_PALETTE] = "palette",
	[MES_SYS_VAR_A6_OFFSET] = "a6_offset",
	[MES_SYS_VAR_FILE_DATA] = "file_data",
	[MES_SYS_VAR_MENU_ENTRY_ADDRESSES] = "menu_entry_addresses",
	[MES_SYS_VAR_MENU_ENTRY_NUMBERS] = "menu_entry_numbers",
	[MES_SYS_VAR_MAP_DATA] = "map_offset",
};

int mes_resolve_sysvar(string name, bool *dword)
{
	for (int i = 0; i < MES_NR_SYSTEM_VARIABLES; i++) {
		if (mes_system_var16_names[i] && !strcmp(name, mes_system_var16_names[i])) {
			*dword = false;
			return i;
		}
		if (mes_system_var32_names[i] && !strcmp(name, mes_system_var32_names[i])) {
			*dword = true;
			return i;
		}
	}
	return -1;
}

LEAF(util, offset_screen);
LEAF(util, blend);
LEAF(util, ending1);
LEAF(util, ending2);
LEAF(util, ending3);

LEAF(util, get_text_colors);
LEAF(util, noop3);
LEAF(util, blink_fade);
LEAF(util, scale_h);
LEAF(util, invert_colors);
LEAF(util, fade);
LEAF(util, pixelate);
LEAF(util, get_time);
LEAF(util, check_cursor);
LEAF(util, delay);
LEAF(util, save_animation);
LEAF(util, restore_animation);
LEAF(util, copy_progressive);
LEAF(util, fade_progressive);
LEAF(util, anim_is_running);
LEAF(util, set_monochrome);
LEAF(util, bgm_play);
LEAF(util, get_ticks);
LEAF(util, wait_until);
LEAF(util, bgm_is_fading);

PUBLIC_NODE(mes_util_isaku, Isaku,
	[0]  = &util_offset_screen,
	[6]  = &util_copy_progressive, // ...probably?
	[7]  = &util_delay,
	[8]  = &util_blend,
	[9]  = &util_ending1,
	[10] = &util_ending2,
	[13] = &util_ending3,
);

PUBLIC_NODE(mes_util_shangrlia, Shangrlia, );

PUBLIC_NODE(mes_util_yuno, YUNO,
	[1]  = &util_get_text_colors,
	[3]  = &util_noop3,
	[5]  = &util_blink_fade,
	[6]  = &util_scale_h,
	[8]  = &util_invert_colors,
	[10] = &util_fade,
	[12] = &util_pixelate,
	[14] = &util_get_time,
	[15] = &util_check_cursor,
	[16] = &util_delay,
	[17] = &util_save_animation,
	[18] = &util_restore_animation,
	[20] = &util_copy_progressive,
	[21] = &util_fade_progressive,
	[22] = &util_anim_is_running,
	[100] = &util_set_monochrome,
	[201] = &util_bgm_play,
	[210] = &util_get_ticks,
	[211] = &util_wait_until,
	[214] = &util_bgm_is_fading,
);

LEAF(util, shift_screen);
LEAF(util, copy_to_surface_7);
LEAF(util, strcpy);
LEAF(util, strcpy2);
LEAF(util, location_index);
LEAF(util, location_zoom);
LEAF(util, get_MESS);
LEAF(util, write_backlog_header);
LEAF(util, line);
LEAF(util, save_voice);
LEAF(util, quit);
LEAF(util, get_IMODE);
LEAF(util, set_prepared_voice);
LEAF(util, cgmode_zoom);
LEAF(util, scroll);
LEAF(util, get_CUT);

PUBLIC_NODE(mes_util_aishimai, AiShimai,
	[0] = &util_shift_screen,
	[1] = &util_copy_to_surface_7,
	[2] = &util_strcpy,
	[3] = &util_strcpy2,
	[4] = &util_location_index,
	[5] = &util_location_zoom,
	[6] = &util_get_MESS,
	[7] = &util_write_backlog_header,
	[8] = &util_line,
	[9] = &util_save_voice,
	[10] = &util_quit,
	[11] = &util_get_IMODE,
	[12] = &util_set_prepared_voice,
	[13] = &util_cgmode_zoom,
	[14] = &util_scroll,
	[15] = NULL,
	[16] = &util_get_CUT,
);

LEAF(util, set_volume);
LEAF(util, get_ini_values);

PUBLIC_NODE(mes_util_beyond, Beyond,
	[0] = NULL,
	[1] = NULL,
	[2] = NULL,
	[3] = NULL,
	[4] = NULL,
	[5] = NULL,
	[6] = NULL,
	[7] = NULL,
	[8] = NULL,
	[9] = NULL,
	[10] = NULL,
	[11] = &util_set_volume,
	[12] = &util_get_ini_values,
	[13] = NULL,
	[14] = NULL,
	[15] = NULL,
	[16] = NULL,
	[17] = NULL,
	[18] = NULL,
	[20] = NULL,
	[21] = NULL,
	[22] = NULL,
	[23] = NULL,
	[24] = NULL,
	[25] = NULL,
	[26] = NULL,
	[27] = NULL,
	[28] = NULL,
	[29] = NULL,
	[30] = NULL,
	[31] = NULL,
	[32] = NULL,
	[33] = NULL,
	[34] = NULL,
	[35] = NULL,
	[36] = NULL,
);

LEAF(util, save_palette);
LEAF(util, restore_palette); // 0-255
LEAF(util, reset_low_palette); // 0-16
LEAF(util, ctrl_is_down);
LEAF(util, get_ccd_sprite_pos);
LEAF(util, activate_is_down);
LEAF(util, wait_until_activate_is_up);
LEAF(util, timer_init);
LEAF(util, timer_set);
LEAF(util, move_wait);
LEAF(util, crossfade_sepia);
LEAF(util, crossfade_high_palette);
LEAF(util, load_player_name);
LEAF(util, save_player_name);
LEAF(util, backlog_add_number);
LEAF(util, mahoko_spin_start);
LEAF(util, mahoko_spin_end);
LEAF(util, save_current_palette);
LEAF(util, backlog_save_ptr);
LEAF(util, backlog_truncate_at_saved_ptr);
LEAF(util, palette_blend_color);
LEAF(util, credits_roll);
LEAF(util, timer_wait);
LEAF(util, bgm_is_playing);
LEAF(util, clock_start);
LEAF(util, clock_get);
LEAF(util, activate_is_down2);
PUBLIC_NODE(mes_util_kakyuusei, Kakyuusei,
	[94] = &util_save_palette,
	[95] = &util_restore_palette,
	[96] = &util_reset_low_palette,
	[97] = &util_ctrl_is_down,
	[98] = &util_get_ccd_sprite_pos,
	[99] = &util_activate_is_down,
	[100] = &util_wait_until_activate_is_up,
	[101] = &util_timer_init,
	[102] = &util_timer_set,
	[103] = &util_move_wait,
	[105] = &util_crossfade_sepia,
	[106] = &util_crossfade_high_palette,
	[107] = &util_load_player_name,
	[111] = &util_backlog_add_number,
	[127] = &util_scroll,
	[128] = &util_quake,
	[129] = &util_mahoko_spin_start,
	[130] = &util_mahoko_spin_end,
	[131] = &util_save_current_palette,
	[133] = &util_save_player_name,
	[134] = &util_backlog_save_ptr,
	[135] = &util_backlog_truncate_at_saved_ptr,
	[136] = &util_palette_blend_color,
	[137] = &util_delay,
	[138] = &util_credits_roll,
	[139] = &util_timer_wait,
	[140] = &util_bgm_is_playing,
	[141] = &util_clock_start,
	[142] = &util_clock_get,
	[143] = NULL, // TODO
	[144] = &util_activate_is_down2,
);

PUBLIC_NODE(mes_util_none, Empty, );

mes_parameter_list mes_resolve_util(mes_qname name)
{
	mes_parameter_list params = vector_initializer;

	if (vector_length(name) < 1)
		goto error;

	struct mes_path_component *ctx = mes_code_tables.util;

	// resolve identifiers
	for (int i = 0; i < vector_length(name); i++) {
		int part_no;
		struct mes_qname_part *part = &kv_A(name, i);
		ctx = _resolve_qname(ctx, part, &part_no);
		if (part_no < 0)
			goto error;
		if (part->type == MES_QNAME_IDENT) {
			string_free(part->ident);
		}
		part->type = MES_QNAME_NUMBER;
		part->number = part_no;
	}

	// create parameter list from resolved identifiers
	for (int i = 0; i < vector_length(name); i++) {
		struct mes_qname_part *part = &kv_A(name, i);
		if (part->type == MES_QNAME_IDENT)
			goto error;
		struct mes_parameter param = {
			.type = MES_PARAM_EXPRESSION,
			.expr = xcalloc(1, sizeof(struct mes_expression))
		};
		if (part->number > 127) {
			param.expr->op = MES_EXPR_IMM16;
			param.expr->arg16 = part->number;
		} else {
			param.expr->op = MES_EXPR_IMM;
			param.expr->arg8 = part->number;
		}
		vector_push(struct mes_parameter, params, param);
	}

	mes_qname_free(name);
	return params;
error:
	vector_destroy(params);
	return (mes_parameter_list)vector_initializer;
}

string mes_get_util_name(mes_parameter_list params, unsigned *skip_params)
{
	*skip_params = 0;
	string name = string_new("Util");
	return get_name(name, mes_code_tables.util, params, skip_params);
}
