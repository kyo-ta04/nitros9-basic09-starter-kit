#!/bin/sh
# Rebuild 6809M.bin + multicomp09_sd.img from the known-good mc09 disk and CamelForth HEX.
set -e
RT="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$RT/.." && pwd)"

# Prefer slim BASIC09 disk if present
DSK="${DSK:-$ROOT/nitros9-runtime/NOS9_mc09_basic09.dsk}"
if [ ! -f "$DSK" ]; then
  DSK="$ROOT/nitros9-mc09-build/level1/mc09/NOS9_6809_L1_DEV_mc09_80d.dsk"
fi
HEX="${HEX:-$ROOT/multicomp6809/multicomp/ROMS/6809/6809M.HEX}"
MANIP="${MANIP:-$ROOT/multicomp6809/bin/nitros9_disk_manip}"

test -f "$DSK" || { echo "missing disk: $DSK" >&2; exit 1; }
test -f "$HEX" || { echo "missing CamelForth HEX: $HEX" >&2; exit 1; }
test -x "$MANIP" || { echo "missing nitros9_disk_manip: $MANIP" >&2; exit 1; }

echo "==> CamelForth ROM from $HEX"
python3 - "$HEX" "$RT/6809M.bin" <<'PY'
import sys
hex_path, out_path = sys.argv[1], sys.argv[2]
data = bytearray([0xFF] * 0x2000)
with open(hex_path) as f:
    for line in f:
        line = line.strip()
        if not line.startswith(":"):
            continue
        ln = int(line[1:3], 16)
        addr = int(line[3:7], 16)
        typ = int(line[7:9], 16)
        if typ != 0:
            continue
        payload = bytes.fromhex(line[9 : 9 + ln * 2])
        if addr >= 0xE000:
            off = addr - 0xE000
            data[off : off + len(payload)] = payload
        elif addr < 0x2000:
            data[addr : addr + len(payload)] = payload
open(out_path, "wb").write(data)
print("wrote", out_path, "size", len(data))
PY

echo "==> Expand OS-9 disk and place at SD offset 80MiB"
tmp=$(mktemp)
perl "$MANIP" "$DSK" -outsd "$tmp"
python3 - "$tmp" "$RT/multicomp09_sd.img" <<'PY'
import os, sys
src, dst = sys.argv[1], sys.argv[2]
disk = open(src, "rb").read()
offset = 80 * 1024 * 1024
with open(dst, "wb") as f:
    f.truncate(offset + len(disk))
    f.seek(offset)
    f.write(disk)
# track 34 of DS80 = LSN 1224 -> SD block 0x28000+0x4C8
with open(dst, "rb") as f:
    f.seek(0x284C8 * 512)
    head = f.read(2)
if head != b"OS":
    raise SystemExit(f"track34 magic bad: {head!r}")
print("wrote", dst, "size", os.path.getsize(dst))
PY
rm -f "$tmp"

printf 'NITROS9\r' > "$RT/multicomp09.bat"
echo "==> Done. Run: $RT/run.sh"
