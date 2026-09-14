#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
[ "$#" -eq 1 ] || { echo "Usage: $0 /absolute/path/to/Japanese-game-data"; exit 1; }
isaku_data=$1
[ -f "$isaku_data/AI5WIN.EXE" ] || { echo "Missing original AI5WIN.EXE"; exit 1; }
dkp=${DEVKITPRO:-/opt/devkitpro}
port=${ISAKU_PORTLIBS:-$dkp/portlibs/switch}
[ -f "$port/lib/pkgconfig/SDL2_ttf.pc" ] || { echo 'Set ISAKU_PORTLIBS to a Switch prefix with SDL2_ttf and sndfile.' >&2; exit 1; }
mkdir -p build-isaku-switch
python3 - "$dkp" "$port" <<'PY'
import pathlib,sys
p,s=sys.argv[1:]
arch=['-D__SWITCH__','-I'+p+'/libnx/include','-I'+s+'/include','-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE','-ftls-model=local-exec']
link=['-specs='+p+'/libnx/switch.specs','-L'+s+'/lib','-L'+p+'/libnx/lib','-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE','-lnx']
pathlib.Path('build-isaku-switch/cross.ini').write_text(f'''[binaries]
c = '{p}/devkitA64/bin/aarch64-none-elf-gcc'
cpp = '{p}/devkitA64/bin/aarch64-none-elf-g++'
ar = '{p}/devkitA64/bin/aarch64-none-elf-ar'
strip = '{p}/devkitA64/bin/aarch64-none-elf-strip'
pkg-config = '{p}/tools/bin/pkg-config'
[built-in options]
c_args = {arch!r}
c_link_args = {link!r}
[host_machine]
system = 'horizon'
cpu_family = 'aarch64'
cpu = 'cortex-a57'
endian = 'little'
[properties]
pkg_config_libdir = ['{s}/lib/pkgconfig', '{p}/portlibs/switch/lib/pkgconfig']
''')
PY
export PKG_CONFIG_PATH=
if [ ! -f build-isaku-switch/build.ninja ]; then
    meson setup build-isaku-switch . --cross-file build-isaku-switch/cross.ini -Dbuildtype=release -Dmovies=disabled -Disaku_switch=true
fi
ninja -C build-isaku-switch
"$dkp/tools/bin/nacptool" --create 'Isaku Renewal JP' 'Isaku Switch Port' '0.1.4' build-isaku-switch/isaku.nacp
python3 switch-src/isaku/extract_cursors.py "$isaku_data/AI5WIN.EXE" build-isaku-switch/isaku-cursors.bin
python3 switch-src/isaku/extract_icon.py "$isaku_data/AI5WIN.EXE" build-isaku-switch/icon.jpg
"$dkp/tools/bin/elf2nro" build-isaku-switch/ai5 build-isaku-switch/isaku.nro --nacp=build-isaku-switch/isaku.nacp --icon=build-isaku-switch/icon.jpg
