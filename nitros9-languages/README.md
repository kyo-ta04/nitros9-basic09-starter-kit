# NitrOS-9 Languages

This repository contains programming languages, compilers, and related tools
for [NitrOS-9](https://github.com/nitros9project/nitros9).

## Contents

- `basic09` — Microware BASIC09 and RunB sources, utilities, and samples
- `bf` — Brainf*ck interpreter and sample programs
- `ccompiler` — Microware C compiler, libraries, headers, and sources
- `pascal` — archival Pascal materials

The Microware source in `basic09/microware` is the authoritative source for
the BASIC09 and RunB modules. Generated or separately disassembled versions
are not maintained as competing sources.

## Building

A neighboring NitrOS-9 checkout is used by default:

```sh
make
```

Set `NITROS9DIR` when the NitrOS-9 checkout is elsewhere:

```sh
make NITROS9DIR=/path/to/nitros9
```

The default target builds BASIC09, Brainf*ck, and the C compiler, including
their disk images. The BASIC09 disk is based on the CoCo 3 floppy recipe in
the NitrOS-9 checkout. Use `make clean` to remove generated modules and disk
images.

The Pascal material is retained for reference and is not part of the
aggregate build.

## Source and licensing

These packages were migrated from the NitrOS-9 `3rdparty/packages` tree.
They retain their original source notices, documentation, and licensing
terms. No repository-wide license is asserted over packages that do not
already include one.
