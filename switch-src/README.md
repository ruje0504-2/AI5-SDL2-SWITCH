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

## Building the libraries devkitPro does not ship (SDL2_ttf, libsndfile)

devkitPro's switch portlibs (`/opt/devkitpro/portlibs/switch`) provide zlib,
libpng, freetype, mesa/EGL, SDL2 and harfbuzz, but **not** SDL2_ttf or
libsndfile — those two (plus their mpg123/lame dependencies) must be built
into your local prefix first. All builds below are aarch64 cross builds done
after `source switch-env.sh` (which sets `CC`, flags and a pkg-config path
covering both the local prefix and devkitPro's portlibs).

    # 1) mpg123 1.31.3  (MP3 decoder; required by libsndfile's MPEG support)
    ./configure --host=aarch64-none-elf --prefix=$PORTLIBS_PREFIX \
        --with-cpu=generic --with-audio=dummy --enable-static --disable-shared
    make -C src/libmpg123 -j$(nproc)          # build ONLY the library
    # pitfalls:
    #  - keep the tarball's build/ aux dir (AC_CONFIG_AUX_DIR); a distcleaned
    #    tree fails with "cannot find install-sh".
    #  - on Apple Silicon macOS hosts replace build/config.guess+config.sub
    #    with modern ones (the 2023 ones do not know arm64-apple-darwin).
    #  - "--with-audio=none" is invalid; use "dummy".
    #  - a plain full "make" fails in the frontend (streamdump.c); only the
    #    library is needed.
    #  - mpg123.h #includes fmt123.h: install BOTH headers.
    cp src/libmpg123/.libs/libmpg123.a $PORTLIBS_PREFIX/lib/
    cp src/libmpg123/mpg123.h src/libmpg123/fmt123.h $PORTLIBS_PREFIX/include/
    cp libmpg123.pc $PORTLIBS_PREFIX/lib/pkgconfig/

    # 2) lame 3.100  (libsndfile's configure enables MPEG only when BOTH
    #                 mpg123 and lame are present)
    ./configure --host=aarch64-none-elf --prefix=$PORTLIBS_PREFIX \
        --disable-shared --enable-static
    make -j$(nproc) && make install            # (same config.sub note as above)

    # 3) libsndfile 1.2.2
    CFLAGS="-std=gnu17 $CFLAGS" ./configure --host=aarch64-none-elf \
        --prefix=$PORTLIBS_PREFIX --disable-shared --enable-static \
        --disable-external-libs --disable-sqlite --disable-alsa
    # verify: "External MPEG Lame/MPG123 : ........... yes"
    make -j$(nproc)                            # library builds; the bundled
    #  tools (sndfile-info & co.) fail to link on libnx (__tls_start) - ignore.
    # pitfalls:
    #  - devkitPro's gcc 15 defaults to C23, where the bundled ALAC code
    #    ("bool"/"false" used as identifiers) does not compile -> -std=gnu17.
    #  - lame must be installed (and lame/lame.h on the include path) or
    #    MPEG is silently disabled.
    cp src/.libs/libsndfile.a $PORTLIBS_PREFIX/lib/
    cp include/sndfile.h $PORTLIBS_PREFIX/include/
    cp sndfile.pc $PORTLIBS_PREFIX/lib/pkgconfig/

    # 4) SDL2_ttf 2.22.0  (apply SDL2_ttf-2.22.0-switch.patch first)
    ./configure --host=aarch64-none-elf --prefix=$PORTLIBS_PREFIX \
        --disable-shared --enable-static \
        --disable-freetype-builtin --disable-harfbuzz
    make -j$(nproc) && make install
    # pitfalls:
    #  - the bundled harfbuzz needs a platform config.h the tarball does not
    #    ship ("#error generate a harfbuzz config for your platform").
    #  - devkitPro's system harfbuzz is built WITHOUT freetype support, so
    #    "--disable-harfbuzz-builtin" alone still fails the configure check;
    #    disable harfbuzz entirely (plain CJK text needs no shaping) and link
    #    devkitPro's freetype2 instead.

## Other build gotchas

- **ffmpeg auto-enable**: devkitPro portlibs ship `libavcodec`/`libavformat`/
  `libavutil`/`libswscale` .pc files, so meson automatically enables movie.c
  (`HAVE_FFMPEG`). The tested Switch configuration does NOT include movie
  playback; to replicate it, exclude `libav*.pc`/`libsw*.pc` from the
  pkg-config search path when running `meson setup` (e.g. via a filtered
  directory, since `[properties] pkg_config_libdir` takes only one value).
- **fonts**: the horizon target forces `embed_fonts`, so ninja embeds
  whatever is in `fonts/`. The repository keeps only the upstream fonts; for
  Chinese text drop a full-CJK `fonts/Kosugi-Regular.ttf` (and optionally
  Noto Sans SC) into the tree BEFORE `meson setup`, or glyphs will be missing
  (see the main README "Fonts" note).
- **host tooling**: `meson`/`ninja` come from Homebrew (`/opt/homebrew/bin`);
  `xxd` is needed for font embedding; the cross file uses devkitPro's
  `pkg-config`.
- **pkg-config search path**: keep BOTH the local prefix and
  `/opt/devkitpro/portlibs/switch/lib/pkgconfig` visible (see the NOTE in
  `switch-env.sh` and the `[properties]` section of `switch-cross.txt`);
  restricting to one directory makes configure/meson fail with
  "dependency ... not found".
