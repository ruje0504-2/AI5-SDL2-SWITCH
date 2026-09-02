# Switch port dependency patches

Sources and patches used to cross-compile the aarch64 (horizon) dependency
chain for the Switch port of AI5-SDL2. Libraries are built from their
official release tarballs, in dependency order:

    zlib → libpng → freetype → libdrm_nouveau → libsndfile (MPEG)
         → mesa → SDL2 → harfbuzz → SDL2_ttf → ai5-sdl2

Toolchain: devkitPro `devkitA64` (`aarch64-none-elf-gcc`) + libnx,
cross-compiled on macOS. See the "Building for Switch" section in the main
README for the full build steps.

## Patches

| File | Applies to | Purpose |
|---|---|---|
| `SDL2-2.28.5.patch` | SDL2 2.28.5 source tree | Nintendo Switch video/joystick/audio/power/thread backends (`__SWITCH__`), horizon `SDL_config.h`, `eglplatform.h` |
| `switch-mesa-20.1.0-5.patch` | mesa 20.1.0-rc3 | Switch EGL/GLX support (from devkitPro pacman-packages) |
| `gl_XML.py.patch`, `glX_XML.py.patch` | mesa 20.1.0-rc3 `src/mapi/glapi/gen/` | Python 3 compatibility fixes for the GL API code generators |
| `OpenGLConfig.cmake` | build helper | cmake config used when building OpenGL/EGL for the switch toolchain |
| `SDL2_ttf-2.22.0-switch.patch` | SDL2_ttf 2.22.0 (`SDL_ttf.c`, `Makefile.in`) | Disable `FT_LOAD_COLOR` and the 64/32-bit aligned blit paths (empty glyphs / empty alpha on aarch64); rewind the shared `SDL_RWops` before loading a font; drop the `showfont`/`glfont` demo programs |
| `mpg123-1.31.3-switch.patch` | mpg123 1.31.3 (`src/compat/compat.c`) | Disable signal catching (newlib lacks a usable sigaction-based catch). **mpg123 + lame are REQUIRED**: libsndfile must be configured with MPEG support or the game has no audio (see Notes below) |

## Notes

- **The game's audio is MP3 despite the `.wav` names**: the entries inside
  `voice.awd`, `bgm.awd` and `se.awd` are named `*.wav` but actually contain
  MPEG audio (MP3) frames. They are decoded through libsndfile, so libsndfile
  MUST be built with MPEG support. When configuring libsndfile, verify the
  summary line "External MPEG Lame/MPG123 : yes" — a build where it says "no"
  (or where mpg123/lame are missing) compiles fine but the resulting game has
  **no voice/BGM/SE at all** (silent audio).
- `SDL2_ttf` ships a `.gitmodules` in its release tarball (freetype as a
  submodule); the switch build links the portlibs freetype instead, so that
  file is removed from the working tree.
- `zlib` needs no source patch (its `zconf.h`/`Makefile` differences are
  just `./configure` output).
- freetype, libpng, libsndfile, lame and libdrm_nouveau build from pristine
  sources.
- The mpg123 patch above is the change actually applied here; the devkitPro
  pacman-packages `switch/mpg123/mpg123-1.31.3.patch` (configure.ac signal
  detection) was downloaded as a reference but was not applied.
