#ifdef ISAKU_SWITCH_PORT
/* Isaku PE cursor compositor. AND/XOR approach follows kawa2-switch-runtime
 * (ef076b4 tools/cursor_panel.inc); resources remain user-owned local data. */
struct native_cursor { unsigned hx, hy; uint8_t pixels[4096]; };
static struct native_cursor *native_cursors;
static SDL_Texture *native_texture;
static bool native_visible = true;
static bool native_enabled;

static void native_cursor_init(void)
{
	if (!isaku_switch_active())
		return;
	FILE *f = fopen("isaku-cursors.bin", "rb");
	if (!f)
		ERROR("Missing isaku-cursors.bin; run switch-src/isaku/extract_cursors.py");
	uint8_t h[12];
	if (fread(h, 1, 12, f) != 12 || memcmp(h, "ISCUR01\0", 8)
			|| le_get32(h, 8) != 54)
		ERROR("Invalid Isaku cursor header");
	native_cursors = xcalloc(54, sizeof(*native_cursors));
	unsigned previous = 0;
	for (unsigned i = 0; i < 54; i++) {
		struct native_cursor *c = &native_cursors[i];
		if (fread(h, 1, 8, f) != 8 || le_get16(h, 0) <= previous
				|| le_get16(h, 6) || le_get16(h, 2) >= 32 || le_get16(h, 4) >= 32
				|| fread(c->pixels, 1, 4096, f) != 4096)
			ERROR("Invalid Isaku cursor record %u", i);
		previous = le_get16(h, 0);
		c->hx = le_get16(h, 2); c->hy = le_get16(h, 4);
		for (unsigned p = 3; p < 4096; p += 4)
			if (c->pixels[p] != 0 && c->pixels[p] != 255)
				ERROR("Invalid cursor AND mask");
	}
	if (fgetc(f) != EOF || ferror(f)) ERROR("Invalid cursor file length");
	fclose(f);
	SDL_CTOR(SDL_CreateTexture, native_texture, gfx.renderer,
		SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, 32, 32);
	SDL_CALL(SDL_SetTextureBlendMode, native_texture, SDL_BLENDMODE_BLEND);
	SDL_CALL(SDL_SetTextureScaleMode, native_texture, SDL_ScaleModeNearest);
	native_enabled = true;
	SDL_ShowCursor(SDL_DISABLE);
}

void cursor_draw(void)
{
	if (!native_enabled || !native_visible) return;
	/* Resource 230 is the arrow used outside an active game cursor. */
	unsigned index = cursor_loaded ? current_cursor + cursor_frame : 30;
	if (index >= 54) return;
	struct native_cursor *c = &native_cursors[index];
	unsigned x, y; cursor_get_pos(&x, &y);
	SDL_Surface *bg = gfx.surface[gfx.screen].scaled ? gfx.scaled_display : gfx.display;
	uint8_t patch[4096] = {0};
	if (SDL_MUSTLOCK(bg)) SDL_CALL(SDL_LockSurface, bg);
	for (int cy = 0; cy < 32; cy++) for (int cx = 0; cx < 32; cx++) {
		int sx = (int)x - (int)c->hx + cx, sy = (int)y - (int)c->hy + cy;
		if (sx < 0 || sy < 0 || sx >= bg->w || sy >= bg->h) continue;
		const uint8_t *mask = c->pixels + (cy * 32 + cx) * 4;
		if (mask[3] == 255 && !(mask[0] | mask[1] | mask[2])) continue;
		const uint8_t *src = (uint8_t*)bg->pixels + sy * bg->pitch + sx * bg->format->BytesPerPixel;
		uint32_t pixel = 0;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		if (bg->format->BytesPerPixel == 3) pixel = src[0] << 16 | src[1] << 8 | src[2];
		else memcpy(&pixel, src, bg->format->BytesPerPixel);
#else
		memcpy(&pixel, src, bg->format->BytesPerPixel);
#endif
		uint8_t rgb[3]; SDL_GetRGB(pixel, bg->format, &rgb[2], &rgb[1], &rgb[0]);
		uint8_t *dst = patch + (cy * 32 + cx) * 4;
		for (int k = 0; k < 3; k++) dst[k] = (rgb[k] & mask[3]) ^ mask[k];
		dst[3] = 255;
	}
	if (SDL_MUSTLOCK(bg)) SDL_UnlockSurface(bg);
	SDL_CALL(SDL_UpdateTexture, native_texture, NULL, patch, 128);
	float scale_x = 1, scale_y = 1;
#ifdef __SWITCH__
	scale_x = (float)GFX_DISPLAY_W / gfx_view.w;
	scale_y = (float)GFX_DISPLAY_H / gfx_view.h;
#endif
	SDL_FRect dst = {((int)x - (int)c->hx) * scale_x,
		((int)y - (int)c->hy) * scale_y, 32 * scale_x, 32 * scale_y};
	SDL_CALL(SDL_RenderCopyF, gfx.renderer, native_texture, NULL, &dst);
}

#else
static const bool native_enabled = false;
static bool native_visible;
static SDL_Texture *native_texture;
static void *native_cursors;
static void native_cursor_init(void) {}
void cursor_draw(void) {}
#endif
