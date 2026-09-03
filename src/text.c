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

#include <SDL_ttf.h>

#ifdef __SWITCH__
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include "nulib.h"
#include "nulib/file.h"
#include "ai5/mes.h"

#include "ai5.h"
#include "game.h"
#include "gfx_private.h"
#include "gfx.h"
#include "memory.h"
#include "swlog.h"

#ifndef AI5_DATA_DIR
#define AI5_DATA_DIR "."
#endif

#ifdef EMBED_DOTGOTHIC
extern unsigned char font_dotgothic[];
extern unsigned int  font_dotgothic_len;
#endif
#ifdef EMBED_KOSUGI
extern unsigned char font_kosugi[];
extern unsigned int  font_kosugi_len;
#endif
#ifdef EMBED_NOTO
extern unsigned char font_noto[];
extern unsigned int  font_noto_len;
#endif
#ifdef EMBED_TAHOMA
extern unsigned char font_tahoma[];
extern unsigned int  font_tahoma_len;
#endif

struct font {
	int size;
	int y_off;
	TTF_Font *id;
	TTF_Font *id_outline;
};

#define MAX_FONTS 256
static struct font fonts[MAX_FONTS] = {0};
static int nr_fonts = 0;
static struct font *cur_font = NULL;

enum font_type {
	FONT_SMALL,
	FONT_LARGE,
	FONT_ENG,
	FONT_UI,
#define NR_FONT_TYPES (FONT_UI+1)
};

struct font_spec {
	bool embedded;
	union {
		struct {
			SDL_RWops *rwops;
			SDL_RWops *rwops_outline;
		};
		char *path;
	};
	unsigned face;
} font_spec[NR_FONT_TYPES] = {0};

bool text_antialias = false;
enum text_shadow_type text_shadow = false;

static struct font *font_lookup(int size)
{
	for (int i = 0; i < nr_fonts; i++) {
		if (fonts[i].size == size)
			return &fonts[i];
	}
	return NULL;
}

#ifdef __SWITCH__
#ifdef EMBED_KOSUGI
static unsigned long diag_rwread(FT_Stream stream, unsigned long offset,
		unsigned char *buffer, unsigned long count)
{
	SDL_RWops *src = (SDL_RWops *)stream->descriptor.pointer;
	SDL_RWseek(src, (int)offset, RW_SEEK_SET);
	if (count == 0)
		return 0;
	return (unsigned long)SDL_RWread(src, buffer, 1, (int)count);
}

static void ft_diag(void)
{
	static bool done = false;
	if (done)
		return;
	done = true;
	FT_Library lib = NULL;
	FT_Face face = NULL;
	FT_Error err = FT_Init_FreeType(&lib);
	sw_log("ft_diag: FT_Init_FreeType err=%d", (int)err);
	if (err)
		return;
	err = FT_New_Memory_Face(lib, font_kosugi, font_kosugi_len, 0, &face);
	sw_log("ft_diag: FT_New_Memory_Face err=%d face=%p", (int)err, (void*)face);
	if (err || !face)
		return;
	sw_log("ft_diag: num_glyphs=%ld family=%s", face->num_glyphs,
		face->family_name ? face->family_name : "?");
	err = FT_Set_Pixel_Sizes(face, 0, 16);
	sw_log("ft_diag: FT_Set_Pixel_Sizes err=%d", (int)err);
	FT_UInt gi = FT_Get_Char_Index(face, 0x4ECE);
	sw_log("ft_diag: U+4ECE glyph_index=%u", gi);
	err = FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT);
	sw_log("ft_diag: FT_Load_Glyph err=%d", (int)err);
	if (!err) {
		FT_GlyphSlot slot = face->glyph;
		int nc = (slot->format == FT_GLYPH_FORMAT_OUTLINE) ? slot->outline.n_contours : -1;
		int np = (slot->format == FT_GLYPH_FORMAT_OUTLINE) ? slot->outline.n_points : -1;
		sw_log("ft_diag: format=%ld n_contours=%d n_points=%d",
			slot->format, nc, np);
		sw_log("ft_diag: metrics bearingY=%ld height=%ld advance=%ld",
			slot->metrics.horiBearingY, slot->metrics.height, slot->metrics.horiAdvance);
		err = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
		int nz = 0;
		for (unsigned r = 0; r < slot->bitmap.rows; r++)
			for (unsigned c = 0; c < slot->bitmap.width; c++)
				if (slot->bitmap.buffer[r * slot->bitmap.pitch + c])
					nz++;
		sw_log("ft_diag: FT_Render_Glyph(NORMAL) err=%d rows=%d width=%d nonzero=%d",
			(int)err, slot->bitmap.rows, slot->bitmap.width, nz);
	}

	// Reproduce SDL2_ttf's stream-based face (SDL_RWFromConstMem + FT_Open_Face)
	SDL_RWops *rw = SDL_RWFromConstMem(font_kosugi, (int)font_kosugi_len);
	sw_log("ft_diag: SDL_RWFromConstMem rw=%p size=%lld",
		(void*)rw, rw ? (long long)SDL_RWsize(rw) : -1);
	if (rw) {
		FT_Stream st = (FT_Stream)calloc(1, sizeof(*st));
		st->size = (unsigned long)SDL_RWsize(rw);
		st->descriptor.pointer = rw;
		st->read = diag_rwread;
		FT_Open_Args args;
		memset(&args, 0, sizeof(args));
		args.flags = FT_OPEN_STREAM;
		args.stream = st;
		FT_Face face2 = NULL;
		err = FT_Open_Face(lib, &args, 0, &face2);
		sw_log("ft_diag(stream): FT_Open_Face err=%d face=%p", (int)err, (void*)face2);
		if (!err && face2) {
			FT_Set_Pixel_Sizes(face2, 0, 16);
			FT_UInt gi2 = FT_Get_Char_Index(face2, 0x4ECE);
			err = FT_Load_Glyph(face2, gi2, FT_LOAD_DEFAULT);
			sw_log("ft_diag(stream): FT_Load_Glyph err=%d", (int)err);
			if (!err) {
				FT_GlyphSlot slot2 = face2->glyph;
				int nc2 = (slot2->format == FT_GLYPH_FORMAT_OUTLINE) ? slot2->outline.n_contours : -1;
				sw_log("ft_diag(stream): n_contours=%d n_points=%d",
					nc2, (slot2->format == FT_GLYPH_FORMAT_OUTLINE) ? slot2->outline.n_points : -1);
				err = FT_Render_Glyph(slot2, FT_RENDER_MODE_NORMAL);
				int nz2 = 0;
				for (unsigned r = 0; r < slot2->bitmap.rows; r++)
					for (unsigned c = 0; c < slot2->bitmap.width; c++)
						if (slot2->bitmap.buffer[r * slot2->bitmap.pitch + c])
							nz2++;
				sw_log("ft_diag(stream): FT_Render_Glyph(NORMAL) err=%d rows=%d width=%d nonzero=%d",
					(int)err, slot2->bitmap.rows, slot2->bitmap.width, nz2);
			// Test FT_LOAD_NO_HINTING (what SDL2_ttf uses after TTF_SetFontHinting(NONE))
			err = FT_Load_Glyph(face2, gi2, FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING);
			sw_log("ft_diag(nohint): FT_Load_Glyph err=%d", (int)err);
			if (!err) {
				FT_GlyphSlot s3 = face2->glyph;
				err = FT_Render_Glyph(s3, FT_RENDER_MODE_NORMAL);
				int nz3 = 0;
				for (unsigned r = 0; r < s3->bitmap.rows; r++)
					for (unsigned c = 0; c < s3->bitmap.width; c++)
						if (s3->bitmap.buffer[r * s3->bitmap.pitch + c])
							nz3++;
				sw_log("ft_diag(nohint): render err=%d rows=%d width=%d nonzero=%d",
					(int)err, s3->bitmap.rows, s3->bitmap.width, nz3);
			}
			// Test FT_Set_Char_Size (what SDL2_ttf uses) + FT_LOAD_DEFAULT
			FT_Set_Char_Size(face2, 0, 16 * 64, 0, 0);
			err = FT_Load_Glyph(face2, gi2, FT_LOAD_DEFAULT);
			if (!err) {
				FT_GlyphSlot s4 = face2->glyph;
				err = FT_Render_Glyph(s4, FT_RENDER_MODE_NORMAL);
				int nz4 = 0;
				for (unsigned r = 0; r < s4->bitmap.rows; r++)
					for (unsigned c = 0; c < s4->bitmap.width; c++)
						if (s4->bitmap.buffer[r * s4->bitmap.pitch + c])
							nz4++;
				sw_log("ft_diag(charsize): render err=%d rows=%d width=%d nonzero=%d",
					(int)err, s4->bitmap.rows, s4->bitmap.width, nz4);
			}
			}
		}
	}
}
#endif
#endif

static struct font *font_insert(int size, TTF_Font *id, TTF_Font *id_outline)
{
#ifdef __SWITCH__
#ifdef EMBED_KOSUGI
	ft_diag();
#endif
#endif
	int min_x, max_x, min_y, max_y, adv;
	int ascent = TTF_FontAscent(id);
#ifdef __SWITCH__
	if (TTF_GlyphMetrics32(id, 'A', &min_x, &max_x, &min_y, &max_y, &adv) != 0 || max_y <= 0) {
		// Switch freetype returns bogus vertical metrics; approximate cap height.
		max_y = size * 11 / 16;
	}
#else
	TTF_GlyphMetrics32(id, 'A', &min_x, &max_x, &min_y, &max_y, &adv);
#endif

	// Calculate the y-offset for the font. This is a bit hacky, but it
	// works reasonably well for most fonts.
	int y_off = ascent - size;         // align baseline to point size
	y_off += (size - (max_y - 2)) / 2; // center based on height of 'A'
	if (game->bpp == 8)
		y_off -= 1;
	else
		y_off -= 2;
	sw_log("font_insert: size=%d ascent=%d y_off=%d max_y=%d adv=%d",
		size, ascent, y_off, max_y, adv);

	if (nr_fonts >= MAX_FONTS)
		ERROR("Font table is full");
	fonts[nr_fonts].size = size;
	fonts[nr_fonts].y_off = y_off;
	fonts[nr_fonts].id = id;
	fonts[nr_fonts].id_outline = id_outline;
	return &fonts[nr_fonts++];
}

#define EMBEDDED_FONT(name) (struct font_spec) { \
	.embedded = true, \
	.rwops = SDL_RWFromConstMem(font_##name, font_##name##_len), \
	.rwops_outline = SDL_RWFromConstMem(font_##name, font_##name##_len), \
}

static void init_ui_font(void)
{
#ifdef EMBED_TAHOMA
	font_spec[FONT_UI] = EMBEDDED_FONT(tahoma);
#else
	font_spec[FONT_UI].path = xstrdup(AI5_DATA_DIR "/fonts/wine_tahoma.ttf");
#endif
}

static void init_fonts_standard(void)
{
#ifdef EMBED_DOTGOTHIC
	font_spec[FONT_SMALL] = EMBEDDED_FONT(dotgothic);
#else
	font_spec[FONT_SMALL].path = xstrdup(AI5_DATA_DIR "/fonts/DotGothic16-Regular.ttf");
#endif
#ifdef EMBED_KOSUGI
	font_spec[FONT_LARGE] = EMBEDDED_FONT(kosugi);
#else
	font_spec[FONT_LARGE].path = xstrdup(AI5_DATA_DIR "/fonts/Kosugi-Regular.ttf");
#endif
#ifdef EMBED_NOTO
	font_spec[FONT_ENG] = EMBEDDED_FONT(noto);
#else
	font_spec[FONT_ENG].path = xstrdup(AI5_DATA_DIR "/fonts/NotoSansJP-Thin.ttf");
#endif
}

void gfx_text_init(const char *font_path, int face)
{
	if (TTF_Init() == -1)
		ERROR("TTF_Init: %s", TTF_GetError());

	init_ui_font();
	if (font_path) {
		// XXX: we override the default face for msgothic on yuno-eng
		int face_eng = face;
		if (!strcasecmp(path_basename(font_path), "msgothic.ttc")) {
			if (face < 0)
				face_eng = 1; // MS PGothic
		}
		if (face < 0) face = 0;
		if (face_eng < 0) face_eng = 0;

		font_spec[FONT_SMALL].path = xstrdup(font_path);
		font_spec[FONT_SMALL].face = face;
		font_spec[FONT_LARGE].path = xstrdup(font_path);
		font_spec[FONT_LARGE].face = face;
		font_spec[FONT_ENG].path = xstrdup(font_path);
		font_spec[FONT_ENG].face = face_eng;
	} else {
#ifdef _WIN32
		if (game->bpp == 8) {
			// XXX: We only use MS Gothic for indexed color, since direct color
			//      games render text with an outline and SDL_ttf can't render
			//      an outline on MS Gothic for some reason.
			font_spec[FONT_SMALL].path = xstrdup("C:/Windows/Fonts/msgothic.ttc");
			font_spec[FONT_LARGE].path = xstrdup(font_spec[FONT_SMALL].path);
			font_spec[FONT_ENG].path = xstrdup(font_spec[FONT_SMALL].path);
			font_spec[FONT_ENG].face = 1;
		} else {
			init_fonts_standard();
		}
#else
		init_fonts_standard();
#endif // _WIN32
	}
	gfx_text_set_size(mem_get_sysvar16(mes_sysvar16_font_height),
			mem_get_sysvar16(mes_sysvar16_font_weight));
}

void gfx_text_set_colors(uint32_t bg, uint32_t fg)
{
	gfx.text.bg = bg;
	gfx.text.fg = fg;
	if (game->bpp == 16) {
		gfx.text.bg_color = gfx_decode_bgr555(bg);
		gfx.text.fg_color = gfx_decode_bgr555(fg);
	}
	else if (game->bpp == 24) {
		gfx.text.bg_color = gfx_decode_bgr(bg);
		gfx.text.fg_color = gfx_decode_bgr(fg);
	}
}

void gfx_text_get_colors(uint32_t *bg, uint32_t *fg)
{
	*bg = gfx.text.bg;
	*fg = gfx.text.fg;
}

void gfx_text_fill(int x, int y, int w, int h, unsigned i)
{
	gfx_fill(x, y, w, h, i, gfx.text.bg);
}

void gfx_text_swap_colors(int x, int y, int w, int h, unsigned i)
{
	gfx_swap_colors(x, y, w, h, i, gfx.text.bg, gfx.text.fg);
}

// XXX: We have to blit manually so that the correct foreground index is written.
static void glyph_blit_indexed(SDL_Surface *glyph, int dst_x, int dst_y, SDL_Surface *s)
{
	int glyph_x = 0;
	int glyph_y = 0;
	int glyph_w = glyph->w;
	int glyph_h = glyph->h;
	if (unlikely(dst_x < 0)) {
		glyph_w += dst_x;
		glyph_x -= dst_x;
		dst_x = 0;
	}
	if (unlikely(dst_y < 0)) {
		glyph_h += dst_y;
		glyph_y -= dst_y;
		dst_y = 0;
	}
	if (unlikely(dst_x + glyph_w > s->w))
		glyph_w = s->w - dst_x;
	if (unlikely(dst_y + glyph_h > s->h))
		glyph_h = s->h - dst_y;
	if (unlikely(glyph_w <= 0 || glyph_h <= 0))
		return;

	// XXX: prevent text from overflowing at bottom
	glyph_h = min(cur_font->y_off + cur_font->size, glyph_h);

	if (SDL_MUSTLOCK(glyph))
		SDL_CALL(SDL_LockSurface, glyph);
	if (SDL_MUSTLOCK(s))
		SDL_CALL(SDL_LockSurface, s);

	// The Switch path renders glyphs as 32-bit ARGB8888 (blended), so alpha
	// is at byte offset 3 and is thresholded to build the 1-bit mask for the
	// 8bpp destination. On other platforms glyphs are 1-byte palette pixels
	// (Solid), matched by the upstream "!= 0" test below.
#ifdef __SWITCH__
	const int glyph_bpp = glyph->format->BytesPerPixel;
	const int a_off = glyph_bpp == 4 ? 3 : 0;
	uint8_t *src_base = glyph->pixels + glyph_y * glyph->pitch + glyph_x * glyph_bpp;
#else
	uint8_t *src_base = glyph->pixels + glyph_y * glyph->pitch + glyph_x;
#endif
	uint8_t *dst_base = s->pixels + dst_y * s->pitch + dst_x;
	for (int row = 0; row < glyph_h; row++) {
		uint8_t *src_p = src_base + row * glyph->pitch;
		uint8_t *dst_p = dst_base + row * s->pitch;
#ifdef __SWITCH__
		for (int col = 0; col < glyph_w; col++, dst_p++, src_p += glyph_bpp) {
			if (src_p[a_off] >= 128) {
#else
		for (int col = 0; col < glyph_w; col++, dst_p++, src_p++) {
			if (*src_p != 0) {
#endif
				*dst_p = gfx.text.fg;
				if (text_shadow == TEXT_SHADOW_A) {
					dst_p[1] = gfx.text.bg;
					dst_p[2] = gfx.text.bg;
					*(dst_p + s->pitch + 1) = gfx.text.bg;
				} else if (text_shadow == TEXT_SHADOW_B) {
					*(dst_p + s->pitch * 2 + 2) = gfx.text.bg;
				}
			}
		}
	}

	if (SDL_MUSTLOCK(s))
		SDL_UnlockSurface(s);
	if (SDL_MUSTLOCK(glyph))
		SDL_UnlockSurface(glyph);
}

static unsigned gfx_text_draw_glyph_indexed(SDL_Surface *dst, int x, int y, uint32_t ch,
		SDL_Rect *damage_out)
{
	assert(gfx.text.fg < dst->format->palette->ncolors);
	SDL_Color fg = dst->format->palette->colors[gfx.text.fg];
#ifdef __SWITCH__
	// Switch freetype's mono (FT_RENDER_MODE_MONO) rasterizer produces empty
	// bitmaps; use grayscale (blended) rendering and threshold the alpha.
	SDL_Surface *s = TTF_RenderGlyph32_Blended(cur_font->id, ch, fg);
	if (!s)
		ERROR("TTF_RenderGlyph32_Blended: %s", TTF_GetError());
	{
		static int gcnt = 0;
		if (gcnt < 3) {
			gcnt++;
			int nz = 0, nz_any = 0;
			for (int r = 0; r < s->h; r++) {
				uint8_t *row = (uint8_t*)s->pixels + r * s->pitch;
				for (int c = 0; c < s->w; c++) {
					if (row[c * 4 + 3]) nz++;
					for (int b = 0; b < 4; b++)
						if (row[c * 4 + b]) { nz_any++; break; }
				}
			}
			uint8_t *p0 = (uint8_t*)s->pixels;
			sw_log("glyph_render: U+%04x w=%d h=%d alpha_nz=%d any_nz=%d fg=%d fga=%d fmt=0x%x bpp=%d pitch=%d",
				ch, s->w, s->h, nz, nz_any, gfx.text.fg, fg.a,
				s->format->format, (int)s->format->BytesPerPixel, s->pitch);
			sw_log("glyph_render: px0=%02x%02x%02x%02x px1=%02x%02x%02x%02x px2=%02x%02x%02x%02x",
				p0[0],p0[1],p0[2],p0[3], p0[4],p0[5],p0[6],p0[7], p0[8],p0[9],p0[10],p0[11]);
		}
	}
#else
	SDL_Surface *s = TTF_RenderGlyph32_Solid(cur_font->id, ch, fg);
	if (!s)
		ERROR("TTF_RenderGlyph32_Solid: %s", TTF_GetError());
#endif

	y -= cur_font->y_off;
	unsigned w = s->w;
	glyph_blit_indexed(s, x, y, dst);
	*damage_out = (SDL_Rect) { x, y, s->w, s->h };
	SDL_FreeSurface(s);
	return w;
}

static unsigned gfx_text_draw_glyph_direct(SDL_Surface *dst, int x, int y, uint32_t ch,
		SDL_Rect *damage_out)
{
	SDL_Surface *outline, *glyph;
	if (!text_antialias) {
		// XXX: Antialiasing can cause issues if the text is rendered to a surface
		//      filled with the mask color and then copied to the main surface with
		//      copy_masked (e.g. Doukyuusei does this).
		outline = TTF_RenderGlyph32_Solid(cur_font->id_outline, ch, gfx.text.bg_color);
		glyph = TTF_RenderGlyph32_Solid(cur_font->id, ch, gfx.text.fg_color);
	} else {
		outline = TTF_RenderGlyph32_Blended(cur_font->id_outline, ch, gfx.text.bg_color);
		glyph = TTF_RenderGlyph32_Blended(cur_font->id, ch, gfx.text.fg_color);
	}
	if (!outline || !glyph)
		ERROR("TTF_RenderGlyph32_Blended: %s", TTF_GetError());

	y -= cur_font->y_off;

	SDL_Rect outline_r = { x-1, y-1, outline->w, outline->h };
	SDL_Rect glyph_r = { x, y, glyph->w, glyph->h };
	SDL_CALL(SDL_BlitSurface, outline, NULL, dst, &outline_r);
	SDL_CALL(SDL_BlitSurface, glyph, NULL, dst, &glyph_r);
	*damage_out = (SDL_Rect) { x-1, y-1, outline->w, outline->h };
	SDL_FreeSurface(glyph);
	SDL_FreeSurface(outline);
	return glyph_r.w;
}

unsigned _gfx_text_draw_glyph(SDL_Surface *dst, int x, int y, uint32_t ch)
{
	if (!cur_font)
		return 0;
	SDL_Rect damage;
	if (game->bpp == 8)
		return gfx_text_draw_glyph_indexed(dst, x, y, ch, &damage);
	return gfx_text_draw_glyph_direct(dst, x, y, ch, &damage);
}

unsigned gfx_text_draw_glyph(int x, int y, unsigned i, uint32_t ch)
{
	if (!cur_font)
		return 0;

	SDL_Surface *dst = gfx_get_surface(i);

	unsigned r;
	SDL_Rect damage;
	if (game->bpp == 8)
		r = gfx_text_draw_glyph_indexed(dst, x, y, ch, &damage);
	else
		r = gfx_text_draw_glyph_direct(dst, x, y, ch, &damage);
	gfx_dirty(i, damage.x, damage.y, damage.w, damage.h);
	return r;
}

#define UI_FONT_SIZE 12
static TTF_Font *ui_font = NULL;
int ui_ascent = 0;

static bool ui_font_init(void)
{
	if (ui_font)
		return true;
	struct font_spec *spec = &font_spec[FONT_UI];
	if (spec->embedded)
		ui_font = TTF_OpenFontIndexRW(spec->rwops, false, UI_FONT_SIZE, spec->face);
	else {
		ui_font = TTF_OpenFontIndex(spec->path, UI_FONT_SIZE, spec->face);
	}
	if (!ui_font) {
		WARNING("TTF_OpenFont: %s", TTF_GetError());
		return false;
	}
	// calculate ASCII ascent based on height of 'A' character.
	int min_x, max_x, min_y, max_y, adv;
	TTF_GlyphMetrics32(ui_font, 'A', &min_x, &max_x, &min_y, &max_y, &adv);
	ui_ascent = max_y;
	return true;
}

void ui_draw_text(SDL_Surface *s, int x, int y, const char *text, SDL_Color color)
{
	if (!ui_font && !ui_font_init()) {
		return;
	}
#ifdef __SWITCH__
	// Switch: mono (Solid) rasterizer renders empty; use grayscale (Blended)
	SDL_Surface *text_s = TTF_RenderUTF8_Blended(ui_font, text, color);
	if (!text_s)
		return;
	SDL_SetSurfaceBlendMode(text_s, SDL_BLENDMODE_BLEND);
#else
	SDL_Surface *text_s = TTF_RenderUTF8_Solid(ui_font, text, color);
#endif
	SDL_Rect text_r = { x, y - (TTF_FontAscent(ui_font) - ui_ascent) - ui_ascent / 2, text_s->w, text_s->h };
	SDL_CALL(SDL_BlitSurface, text_s, NULL, s, &text_r);
	SDL_FreeSurface(text_s);
}

int ui_measure_text(const char *text)
{
	if (!ui_font && !ui_font_init())
		return 0;

	int extent, count;
	if (TTF_MeasureUTF8(ui_font, text, 10000, &extent, &count)) {
		WARNING("TTF_MeasureUTF8: %s", TTF_GetError());
		return 0;
	}
	return extent;
}

static void open_font(struct font_spec *spec, int size, TTF_Font **out, TTF_Font **outline_out)
{
	if (spec->embedded) {
		*out = TTF_OpenFontIndexRW(spec->rwops, false, size, spec->face);
		*outline_out = TTF_OpenFontIndexRW(spec->rwops_outline, false, size, spec->face);
	} else {
		*out = TTF_OpenFontIndex(spec->path, size, spec->face);
		*outline_out = TTF_OpenFontIndex(spec->path, size, spec->face);
	}
	if (!*out || !*outline_out)
		ERROR("TTF_OpenFont: %s", TTF_GetError());
	TTF_SetFontOutline(*outline_out, 1);
}

void gfx_text_set_size(int size, int weight)
{
	struct font *font = font_lookup(size);
	if (!font) {
		TTF_Font *f;
		TTF_Font *f_outline = NULL;
#ifdef __SWITCH__
		// The Switch ships a single embedded font; do not pick FONT_SMALL.
		enum font_type type = yuno_eng ? FONT_ENG : FONT_LARGE;
#else
		enum font_type type = yuno_eng ? FONT_ENG : (size <= 18 ? FONT_SMALL : FONT_LARGE);
#endif
		open_font(&font_spec[type], size, &f, &f_outline);
		font = font_insert(size, f, f_outline);
	}
	int style = weight ? TTF_STYLE_BOLD : TTF_STYLE_NORMAL;
	TTF_SetFontStyle(font->id, style);
	TTF_SetFontStyle(font->id_outline, style);
	cur_font = font;
	gfx.text.size = size;
}

void gfx_text_set_weight(int weight)
{
	int style = weight ? TTF_STYLE_BOLD : TTF_STYLE_NORMAL;
	if (TTF_GetFontStyle(cur_font->id) != style) {
		TTF_SetFontStyle(cur_font->id, style);
		TTF_SetFontStyle(cur_font->id_outline, style);
	}
}

unsigned gfx_text_size_char(uint32_t ch)
{
	int minx, maxx, miny, maxy, advance;
	TTF_GlyphMetrics32(cur_font->id, ch, &minx, &maxx, &miny, &maxy, &advance);
	return advance;
}
