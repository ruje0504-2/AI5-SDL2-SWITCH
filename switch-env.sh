#!/usr/bin/env bash
# Cross-compilation environment for building ai5-sdl2 for Nintendo Switch.
# Installs the cross-compiled portlibs into a workspace prefix
# (switch-portlibs/) instead of writing into /opt/devkitpro.
# Usage: source switch-env.sh

export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export DEVKITARM=$DEVKITPRO/devkitARM
export PATH="$DEVKITPRO/tools/bin:$DEVKITA64/bin:/opt/homebrew/bin:$PATH"

# Writable workspace prefix where cross-compiled libraries are installed
export PORTLIBS_PREFIX="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/switch-portlibs"
export PORTLIBS_PATH="$(dirname "$PORTLIBS_PREFIX")"
export PORTLIBS="$PORTLIBS_PREFIX"

export PKG_CONFIG_PATH="$PORTLIBS_PREFIX/lib/pkgconfig"
export PKG_CONFIG_LIBDIR="$PORTLIBS_PREFIX/lib/pkgconfig"
export PYTHONPATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/python-pkgs:${PYTHONPATH:-}"

# Codegen flags matching devkitPro's official values (libraries use -fPIC;
# switch to -fPIE for the final .nro link)
export ARCH="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec"
export CFLAGS="-g -O2 -ffunction-sections -fdata-sections $ARCH"
export CXXFLAGS="$CFLAGS -fno-rtti -fno-exceptions"
export CPPFLAGS="-D__SWITCH__ -I$DEVKITPRO/libnx/include -I$PORTLIBS_PREFIX/include"
export LDFLAGS="-specs=$DEVKITPRO/libnx/switch.specs -g $ARCH -L$DEVKITPRO/libnx/lib -L$PORTLIBS_PREFIX/lib"
export LIBS="-lnx"

export CC=aarch64-none-elf-gcc
export CXX=aarch64-none-elf-g++
export AR=aarch64-none-elf-ar
export RANLIB=aarch64-none-elf-ranlib
export STRIP=aarch64-none-elf-strip
export HOST=aarch64-none-elf

mkdir -p "$PORTLIBS_PREFIX/include" "$PORTLIBS_PREFIX/lib" "$PORTLIBS_PREFIX/bin" "$PORTLIBS_PREFIX/licenses"
echo "[switch-env] PORTLIBS_PREFIX=$PORTLIBS_PREFIX"
