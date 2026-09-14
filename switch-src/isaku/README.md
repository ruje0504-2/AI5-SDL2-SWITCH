# Isaku Renewal on Switch (opt-in)

This port is disabled by default. Build with `-Disaku_switch=true` for Horizon
to enable it. Standard desktop builds and standard Switch builds keep the
existing implementations, including Shuusaku's data-directory search. The
special build additionally checks the selected game before using shared input,
rendering, font, cursor, popup-menu or save changes.

## Build

Use devkitPro/devkitA64, libnx and Switch portlibs providing SDL2, SDL2_ttf,
libsndfile (with the codecs required by your data), mpg123 and libpng. Set
`ISAKU_PORTLIBS` if these dependencies are not in `$DEVKITPRO/portlibs/switch`.
Python 3 and either macOS `sips` or ImageMagick `magick` are needed to extract and
convert the user's original EXE icon. No original data or fonts are downloaded.

```sh
ISAKU_PORTLIBS=/absolute/path/to/switch-portlibs \
  ./switch-src/isaku/build.sh /absolute/path/to/original-japanese-game
```

Outputs are in `build-isaku-switch/`: `isaku.nro`, `isaku-cursors.bin` and the
extracted icon. Copy the NRO and cursor file alongside your own game resources
in `sdmc:/switch/isaku/`. Launch from hbmenu in full application mode.

The script enables `isaku_switch` and disables optional FMV dependencies for
this title. The new `movies` feature option defaults to `auto`, preserving the
repository's existing dependency detection for normal builds.

## Japanese data

Keep the original AI5WIN.EXE/AI5WIN.INI and MES, BG, DATA, MUSIC, VOICE archives.
The original executable is used for resources, not executed on the Switch.
Original cursor/icon extraction is performed locally; generated resources are
not part of this source update.

## Existing Chinese patch

Use your own CHS.ARC and HATA.ARC and an appropriate Chinese font. In the normal
AI5WIN.INI set these values (retain all other original sections/settings):

```ini
[Config]
TITLE=Isaku98
StartMES=MAIN.MES
TEXTENCODING=GBK
[FILE]
ARCMESNAME=CHS.ARC
ARCDATANAME=HATA.ARC
[AI5SDL2]
FONT=fonts/your-chinese-font.ttf
[CONTROLLER]
ENABLED=1
UI=1
LEFTANALOG=CURSOR
RIGHTANALOG=CURSOR
BACK=S
START=L
LEFTTRIGGER=SHIFT
RIGHTTRIGGER=TAB
```

The existing patch text is not redistributed here. GBK/ASCII mixed text,
Chinese punctuation and the on-screen menu font are enabled only for the
opt-in Isaku port. `subprojects/libai5` remains unchanged; development-only
static parser changes are not included in this update.

## Controls and saves

Both sticks move the pointer. A confirms, B cancels, Y toggles inventory,
X skips text, ZL skips waits, ZR toggles message visibility, minus saves, plus
loads, and right-stick click opens the menu. USB mouse input shares the pointer.
Existing explicit controller mappings are respected.

Preserve every FLAG* save. The port creates missing files from the clean Isaku
initialization template instead of all-zero data. It repairs only recognizable
empty initialization tables while reading, preserving progress and leaving
the original file untouched. Valid saved mute settings remain respected.

## Scope and validation

`include/isaku_switch.h` owns the build/game gate. The dedicated Isaku function
variants are compiled only when that gate is enabled. Other games retain the
original shared branches even if launched explicitly with the opt-in binary.

Host regression builds may explicitly define both `AI5_ISAKU_SWITCH_BUILD` and
`AI5_ISAKU_SWITCH_TEST`. The latter is a private test escape hatch, never a
production desktop default. See `tests/isaku-switch/README.md`.

Original effects are not claimed to be completely identical to the PC engine.
The remaining known differences include the special ending animation and some
CPU-dependent timings. These are outside this update's isolation work.

## Upstream comparison (2026-09-14)

Prepared against upstream master `94eba0c0009d154aeb13ab2c64f05f98f1f2636d`.
The shared input/rendering/cursor/menu/save/text paths use the Isaku gate;
Isaku-specific effect implementations have the original implementation as the
normal-build fallback. No other game's source, libai5 source, bundled font or
existing platform workflow is changed.

Local verification passed for ordinary macOS, the explicit host test build,
normal Horizon and opt-in Horizon builds. The public build script also produced
an NRO using locally supplied original resources. Default/non-target isolation,
save initialization and migration, GBK text boundaries, transition pixels,
refresh pixels, cursor composition and menu callbacks passed regression checks.
These checks do not replace console playthrough testing. The added Linux CI
workflow runs the data-free regression fixtures after publication.

## Attribution

AI5-SDL2 and this update use the repository's GPL license. The cursor resource
extraction and AND/XOR composition approach was adapted from
[ruje0504-2/kawa2-switch-runtime](https://github.com/ruje0504-2/kawa2-switch-runtime)
revision `ef076b486ad9a14052402cfa79d39178e7ecdf93` (GPLv2). All generated original
game resources remain in the user's local data directory.
