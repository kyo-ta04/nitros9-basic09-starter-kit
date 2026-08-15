#!/bin/sh
# Build a slim NitrOS-9 Level 1 mc09 disk for BASIC09 programming.
# Strips unrelated CMDS and trims the bootfile (no pipes, no extra ports).
set -e

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
NITROS9DIR="${NITROS9DIR:-$ROOT/nitros9-mc09-build}"
LANGUAGES="${LANGUAGES:-$ROOT/nitros9-languages}"
MD="$NITROS9DIR/level1/mc09/modules"
BF="$NITROS9DIR/level1/mc09/bootfiles"
CMDS="$NITROS9DIR/level1/mc09/cmds"
OUT="${1:-$ROOT/nitros9-runtime/NOS9_mc09_basic09.dsk}"
B09="$LANGUAGES/basic09"

export PATH="/usr/local/bin:${PATH}"
export NITROS9DIR

test -d "$NITROS9DIR" || { echo "missing NITROS9DIR=$NITROS9DIR" >&2; exit 1; }
test -f "$B09/basic09_6809" || {
  echo "building BASIC09..."
  make -C "$B09" basic09_6809 runb_6809 inkey syscall
}
test -f "$BF/kernel_mc09sd" || make -C "$NITROS9DIR" dsk PORTS=mc09

# Ensure core modules exist
for m in ioman rbf.mn dds0_80d.dd mc09sdc.dr s0_80d.dd scf.mn \
         mc6850.dr term_mc6850.dt mc09clock_50hz clock2_soft sysgo_dd; do
  test -f "$MD/$m" || { echo "missing module $MD/$m -- run: make -C $NITROS9DIR dsk PORTS=mc09" >&2; exit 1; }
done
test -f "$CMDS/shell_21" || { echo "missing shell_21" >&2; exit 1; }

echo "==> slim bootfile (no pipe, no extra T* ports, single drive)"
cat \
  "$MD/ioman" \
  "$MD/rbf.mn" \
  "$MD/dds0_80d.dd" \
  "$MD/mc09sdc.dr" \
  "$MD/s0_80d.dd" \
  "$MD/scf.mn" \
  "$MD/mc6850.dr" \
  "$MD/term_mc6850.dt" \
  "$MD/mc09clock_50hz" \
  "$MD/clock2_soft" \
  "$MD/sysgo_dd" \
  > /tmp/bootfile_mc09_basic09
ls -la /tmp/bootfile_mc09_basic09

echo "==> format + os9gen $OUT"
rm -f "$OUT"
os9 format -e -t80 -ds -dd -q "$OUT" -n"NitrOS-9 L1 BASIC09"
os9 gen "$OUT" -b=/tmp/bootfile_mc09_basic09 -t="$BF/kernel_mc09sd"

os9 makdir "$OUT,CMDS"
os9 makdir "$OUT,SYS"

# Minimal CMDS for setime + BASIC09 programming
MIN_CMDS="shell_21 setime date echo mfree dir list free load unlink attr copy del"
for c in $MIN_CMDS; do
  test -f "$CMDS/$c" || { echo "missing cmd $c" >&2; exit 1; }
  os9 copy -o=0 "$CMDS/$c" "$OUT,CMDS/$c"
  os9 attr -q -pe -npw -pr -e -w -r "$OUT,CMDS/$c"
done
os9 rename "$OUT,CMDS/shell_21" shell

# BASIC09 package (no gfx)
os9 copy -o=0 "$B09/basic09_6809" "$OUT,CMDS/basic09"
os9 copy -o=0 "$B09/runb_6809" "$OUT,CMDS/runb"
os9 copy -o=0 "$B09/inkey" "$OUT,CMDS/inkey"
os9 copy -o=0 "$B09/syscall" "$OUT,CMDS/syscall"
for c in basic09 runb inkey syscall; do
  os9 attr -q -pe -npw -pr -e -w -r "$OUT,CMDS/$c"
done

# Minimal startup: setime then drop to shell (user runs basic09)
cat > /tmp/startup.basic09 <<'EOF'
* Slim startup for BASIC09
echo * NitrOS-9 L1 mc09 / BASIC09 *
setime </term
date -t
echo * Type: basic09 *
EOF
os9 copy -o=0 -l /tmp/startup.basic09 "$OUT,startup"
os9 attr -q -npe -npw -pr -ne -w -r "$OUT,startup"

# Tiny errmsg (optional -- date/setime may want it)
if [ -f "$NITROS9DIR/level1/sys/errmsg" ]; then
  os9 copy -o=0 -l "$NITROS9DIR/level1/sys/errmsg" "$OUT,SYS/errmsg" 2>/dev/null || true
fi

echo "==> disk contents"
os9 dir "$OUT,"
os9 dir "$OUT,CMDS"
os9 free "$OUT,"
os9 ident -s "$OUT,CMDS/basic09" 2>/dev/null || os9 ident "$OUT,CMDS/basic09" | head -12

echo "==> done: $OUT"
echo "Next: DSK=$OUT $ROOT/nitros9-runtime/rebuild-runtime.sh"
echo "      (or point rebuild at this dsk)"
