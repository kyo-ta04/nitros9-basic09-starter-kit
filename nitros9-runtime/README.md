# NitrOS-9 multicomp09 runtime (boot → Time?)

Clean runtime layout. Rebuilt after workspace cleanup (2026-08-09).

## What this is

| Item | Role |
|------|------|
| `6809M.bin` | CamelForth 8K ROM (loads at `$E000`) |
| `multicomp09_sd.img` | SD card image; NitrOS-9 disk0 at **80 MiB** |
| `multicomp09.bat` | Auto-types `NITROS9` into CamelForth |
| `run.sh` | One-shot boot |

**Simulator:** `~/6809/exec09/m6809-run -s multicomp09`  
**OS build tree:** `~/6809/nitros9-mc09-build` branch `wip/mc09-banner-polled`  
**Upstream clean tree:** `~/6809/nitros9` branch `main` (untouched)

## Run

### Interactive (manual keyboard at Time?)

```bash
~/6809/nitros9-runtime/run.sh
```

At `Time ?` type e.g. `2026/08/09 12:00:00` and ENTER.

`run.sh` passes **`-m 0`** so the simulator does not stop after ~2e9 cycles
while you type (that used to look like “time up” mid-line).

### Automated (PTY feeds the date, stops at shell)

```bash
~/6809/nitros9-runtime/boot-to-shell.py
# optional: NITROS9_DATE='2026/08/09 12:00:00' ./boot-to-shell.py
```

Expected:

```text
...
Time ?
...
Shell
OS9:
```

Console is **polled** 6850-style UART (no UART IRQ). System clock still uses the 50 Hz timer IRQ.

## BASIC09 (programming image)

Slim disk focused on BASIC09 (extra CMDS/boot modules stripped):

```bash
# rebuild OS disk + SD image
./build-basic09-disk.sh
./rebuild-runtime.sh

# boot
./run.sh -m 0    # already includes -m 0
# at OS9:
#   mfree
#   basic09
# → Ready / B:
```

CMDS on the slim disk: `shell setime date echo mfree dir list free load unlink attr copy del basic09 runb inkey syscall`.

## Regenerate artifacts (if deleted)

```bash
~/6809/nitros9-runtime/rebuild-runtime.sh
```

Requires:

- existing boot disk:  
  `nitros9-mc09-build/level1/mc09/NOS9_6809_L1_DEV_mc09_80d.dsk`
- CamelForth hex:  
  `multicomp6809/multicomp/ROMS/6809/6809M.HEX`
- `nitros9_disk_manip` from multicomp6809
- `m6809-run` built in exec09

## Full OS disk rebuild (from source)

```bash
export NITROS9DIR=~/6809/nitros9-mc09-build
export PATH="/usr/local/bin:$PATH"
cd "$NITROS9DIR"
# first time / after clean: ensure defsfile links
ln -sfn mc09/defsfile level1/defsfile
ln -sfn ../mc09/defsfile level1/cmds/defsfile
ln -sfn ../mc09/defsfile level1/modules/defsfile
make dsk PORTS=mc09
# then:
./nitros9-runtime/rebuild-runtime.sh
```

Patches that make banner work live only on branch **`wip/mc09-banner-polled`**
(polled `mc6850`, modular `mc09clock`).

## Layout of repos (after clean)

```text
~/6809/
  nitros9/                 upstream main (clean)
  nitros9-mc09-build/      worktree + branch wip/mc09-banner-polled
  nitros9-runtime/         ROM + SD + run scripts  ← you are here
  exec09/                  simulator (m6809-run)
  multicomp6809/           CamelForth hex, disk tools
  toolshed/                os9 host tools (installed to /usr/local)
  lwtools-4.25/            assembler
```
