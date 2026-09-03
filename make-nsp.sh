#!/usr/bin/env bash
# 打包臭作为完整 NSP（游戏数据进 RomFS，存档走 HOS SaveData）。
#
# 前置:
#   - /opt/devkitpro 已安装（devkitA64 + libnx + tools）
#   - ~/.switch/prod.keys 存在（hacbrewpack 需要）
#   - ~/bin/hacbrewpack(.real) 存在
#   - 本目录下需有游戏数据目录 syuusaku/（*.arc/*.awd/*.ini；数据有版权，
#     不入库，需自行提供；不含运行时生成的 flag*/save）
#   - main.jpg（NSP 图标源图）
#   - 脚本会自动运行 ninja -C ai5-sdl2/build 编译
#
# 用法: ./make-nsp.sh [输出路径, 默认 ./$TITLE_ID.nsp]
set -e
cd "$(dirname "$0")"

TITLE_ID="0100E887ADE40000"
OUT="${1:-$PWD/$TITLE_ID.nsp}"
WS="$PWD"
BUILD="$WS/ai5-sdl2/build"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$TMP/exefs" "$TMP/control" "$TMP/romfs"

echo "[0/5] 交叉编译（ninja -C ai5-sdl2/build）"
source /opt/devkitpro/devkita64.sh 2>/dev/null || true
export PATH="/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:$PATH"
ninja -C "$BUILD" || { echo "编译失败"; exit 1; }

echo "[1/5] strip + elf2nso"
/opt/devkitpro/devkitA64/bin/aarch64-none-elf-strip "$BUILD/ai5" -o "$BUILD/ai5.strip"
/opt/devkitpro/tools/bin/elf2nso "$BUILD/ai5.strip" "$TMP/exefs/main"

echo "[2/5] nacp + npdm"
/opt/devkitpro/tools/bin/nacptool --create "臭作" "elf" "1.0.0" "$TMP/control/control.nacp"
cat > "$TMP/npdm.json" <<'EOF'
{
  "name": "Shuusaku",
  "title_id": "0x0100E887ADE40000",
  "title_id_range_min": "0x0100E887ADE40000",
  "title_id_range_max": "0x01ffffffffffffff",
  "main_thread_stack_size": "0x100000",
  "main_thread_priority": 44,
  "default_cpu_id": 0,
  "process_category": 0,
  "pool_partition": 0,
  "is_64_bit": true,
  "address_space_type": 1,
  "is_retail": true,
  "filesystem_access": {
    "permissions": "0xFFFFFFFFFFFFFFFF"
  },
  "service_host": [
    "*"
  ],
  "service_access": [
    "*"
  ],
  "kernel_capabilities": [
    {
      "type": "kernel_flags",
      "value": {
        "highest_thread_priority": 59,
        "lowest_thread_priority": 28,
        "highest_cpu_id": 2,
        "lowest_cpu_id": 0
      }
    },
    {
      "type": "syscalls",
      "value": {
        "svcUnknown00": "0x00",
        "svcSetHeapSize": "0x01",
        "svcSetMemoryPermission": "0x02",
        "svcSetMemoryAttribute": "0x03",
        "svcMapMemory": "0x04",
        "svcUnmapMemory": "0x05",
        "svcQueryMemory": "0x06",
        "svcExitProcess": "0x07",
        "svcCreateThread": "0x08",
        "svcStartThread": "0x09",
        "svcExitThread": "0x0A",
        "svcSleepThread": "0x0B",
        "svcGetThreadPriority": "0x0C",
        "svcSetThreadPriority": "0x0D",
        "svcGetThreadCoreMask": "0x0E",
        "svcSetThreadCoreMask": "0x0F",
        "svcGetCurrentProcessorNumber": "0x10",
        "svcSignalEvent": "0x11",
        "svcClearEvent": "0x12",
        "svcMapSharedMemory": "0x13",
        "svcUnmapSharedMemory": "0x14",
        "svcCreateTransferMemory": "0x15",
        "svcCloseHandle": "0x16",
        "svcResetSignal": "0x17",
        "svcWaitSynchronization": "0x18",
        "svcCancelSynchronization": "0x19",
        "svcArbitrateLock": "0x1A",
        "svcArbitrateUnlock": "0x1B",
        "svcWaitProcessWideKeyAtomic": "0x1C",
        "svcSignalProcessWideKey": "0x1D",
        "svcGetSystemTick": "0x1E",
        "svcConnectToNamedPort": "0x1F",
        "svcSendSyncRequestLight": "0x20",
        "svcSendSyncRequest": "0x21",
        "svcSendSyncRequestWithUserBuffer": "0x22",
        "svcSendAsyncRequestWithUserBuffer": "0x23",
        "svcGetProcessId": "0x24",
        "svcGetThreadId": "0x25",
        "svcBreak": "0x26",
        "svcOutputDebugString": "0x27",
        "svcReturnFromException": "0x28",
        "svcGetInfo": "0x29",
        "svcFlushEntireDataCache": "0x2A",
        "svcFlushDataCache": "0x2B",
        "svcMapPhysicalMemory": "0x2C",
        "svcUnmapPhysicalMemory": "0x2D",
        "svcGetFutureThreadInfo": "0x2E",
        "svcGetLastThreadInfo": "0x2F",
        "svcGetResourceLimitLimitValue": "0x30",
        "svcGetResourceLimitCurrentValue": "0x31",
        "svcSetThreadActivity": "0x32",
        "svcGetThreadContext3": "0x33",
        "svcWaitForAddress": "0x34",
        "svcSignalToAddress": "0x35",
        "svcUnknown36": "0x36",
        "svcUnknown37": "0x37",
        "svcUnknown38": "0x38",
        "svcUnknown39": "0x39",
        "svcUnknown3a": "0x3A",
        "svcUnknown3b": "0x3B",
        "svcDumpInfo": "0x3C",
        "svcDumpInfoNew": "0x3D",
        "svcUnknown3e": "0x3E",
        "svcUnknown3f": "0x3F",
        "svcCreateSession": "0x40",
        "svcAcceptSession": "0x41",
        "svcReplyAndReceiveLight": "0x42",
        "svcReplyAndReceive": "0x43",
        "svcReplyAndReceiveWithUserBuffer": "0x44",
        "svcCreateEvent": "0x45",
        "svcUnknown46": "0x46",
        "svcUnknown47": "0x47",
        "svcMapPhysicalMemoryUnsafe": "0x48",
        "svcUnmapPhysicalMemoryUnsafe": "0x49",
        "svcSetUnsafeLimit": "0x4A",
        "svcCreateCodeMemory": "0x4B",
        "svcControlCodeMemory": "0x4C",
        "svcSleepSystem": "0x4D",
        "svcReadWriteRegister": "0x4E",
        "svcSetProcessActivity": "0x4F",
        "svcCreateSharedMemory": "0x50",
        "svcMapTransferMemory": "0x51",
        "svcUnmapTransferMemory": "0x52",
        "svcDebugActiveProcess": "0x60",
        "svcBreakDebugProcess": "0x61",
        "svcTerminateDebugProcess": "0x62",
        "svcGetDebugEvent": "0x63",
        "svcContinueDebugEvent": "0x64",
        "svcGetProcessList": "0x65",
        "svcGetThreadList": "0x66",
        "svcGetDebugThreadContext": "0x67",
        "svcSetDebugThreadContext": "0x68",
        "svcQueryDebugProcessMemory": "0x69",
        "svcReadDebugProcessMemory": "0x6A",
        "svcWriteDebugProcessMemory": "0x6B",
        "svcSetHardwareBreakPoint": "0x6C",
        "svcGetDebugThreadParam": "0x6D",
        "svcConnectToPort": "0x72",
        "svcSetProcessMemoryPermission": "0x73",
        "svcMapProcessMemory": "0x74",
        "svcUnmapProcessMemory": "0x75",
        "svcQueryProcessMemory": "0x76",
        "svcMapProcessCodeMemory": "0x77",
        "svcUnmapProcessCodeMemory": "0x78"
      }
    },
    {
      "type": "application_type",
      "value": 1
    },
    {
      "type": "min_kernel_version",
      "value": "0x30"
    },
    {
      "type": "handle_table_size",
      "value": 512
    },
    {
      "type": "debug_flags",
      "value": {}
    }
  ],
  "program_id": "0x0100E887ADE40000",
  "program_id_range_min": "0x0100E887ADE40000",
  "program_id_range_max": "0x01ffffffffffffff"
}
EOF
/opt/devkitpro/tools/bin/npdmtool "$TMP/npdm.json" "$TMP/exefs/main.npdm"

echo "[3/5] icon（main.jpg 底部正方形 → 256x256）"
python3 - "$WS/main.jpg" "$TMP/control/icon_AmericanEnglish.dat" <<'PYEOF'
from PIL import Image
import sys
im = Image.open(sys.argv[1]).convert('RGB')
w, h = im.size
# 底部正方形: 下边=图片底边, 裁掉上半部
box = (0, h - w, w, h)
img = im.crop(box).resize((256, 256), Image.LANCZOS)
img.save(sys.argv[2], 'JPEG', quality=90)
print(f"icon: main.jpg {w}x{h} bottom-crop -> 256x256")
PYEOF

echo "[4/5] 拷贝只读游戏数据到 RomFS（不含 flag*/save）"
for f in "$WS/syuusaku/"*.arc "$WS/syuusaku/"*.awd "$WS/syuusaku/"*.ini; do
    [ -e "$f" ] && cp "$f" "$TMP/romfs/"
done
du -sh "$TMP/romfs" | awk '{print "romfs:", $1}'

echo "[5/5] hacbrewpack 打包 NSP"
cd "$TMP"
"$HOME/bin/hacbrewpack" --keyset "$HOME/.switch/prod.keys" \
    --titleid "$TITLE_ID" --titlename "臭作" --titlepublisher "elf" \
    --nologo >/dev/null
cp "hacbrewpack_nsp/$TITLE_ID.nsp" "$OUT"
ls -la "$OUT" | awk '{print "NSP:", $5, $NF}'
echo "完成: $OUT"
