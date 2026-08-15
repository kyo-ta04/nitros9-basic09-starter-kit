# NitrOS-9 multicomp09 ランタイム (ブート → Time?）

クリーンなランタイム構成。ワークスペースのクリーンアップ後に再構築済み（2026-08-09）。

## これは何か

| 項目 | 役割 |
|------|------|
| `6809M.bin` | CamelForth 8K ROM（`$E000` にロード） |
| `multicomp09_sd.img` | SDカードイメージ。NitrOS-9 ディスク0 は **80 MiB** |
| `multicomp09.bat` | CamelForth に `NITROS9` を自動入力 |
| `run.sh` | 1回限りのブート |

**シミュレータ:** `~/6809/exec09/m6809-run -s multicomp09`  
**OS ビルドツリー:** `~/6809/nitros9-mc09-build` ブランチ `wip/mc09-banner-polled`  
**上流のクリーンツリー:** `~/6809/nitros9` ブランチ `main`（未変更）

## 実行

### インタラクティブ（`Time ?` で手動入力）

```bash
~/6809/nitros9-runtime/run.sh
```

`Time ?` で例えば `2026/08/09 12:00:00` と入力して ENTER。

`run.sh` は **`-m 0`** を渡すので、入力中に約 2e9 サイクルでシミュレータが停止することがなくなる（以前は途中で「time up」のようになることがあった）。

### 自動化（PTY が日付を送り、シェルで停止）

```bash
~/6809/nitros9-runtime/boot-to-shell.py
# オプション: NITROS9_DATE='2026/08/09 12:00:00' ./boot-to-shell.py
```

期待される出力:

```text
...
Time ?
...
Shell
OS9:
```

コンソールは 6850 互換 UART のポーリング（UART IRQ なし）で動作。システムクロックは 50 Hz タイマー IRQ を引き続き使用。

## BASIC09（プログラミング用イメージ）

BASIC09 に特化したスリムディスク（余分な CMDS / ブートモジュールを取り除いたもの）:

```bash
# OSディスク + SDイメージを再構築
./build-basic09-disk.sh
./rebuild-runtime.sh

# ブート
./run.sh -m 0    # すでに -m 0 が含まれている
# OS9 で:
#   mfree
#   basic09
# → Ready / B:
```

スリムディスクに含まれる CMDS: `shell setime date echo mfree dir list free load unlink attr copy del basic09 runb inkey syscall`。

## 成果物を再生成する（削除された場合）

```bash
~/6809/nitros9-runtime/rebuild-runtime.sh
```

必要なもの:

- 既存のブートディスク:  
  `nitros9-mc09-build/level1/mc09/NOS9_6809_L1_DEV_mc09_80d.dsk`
- CamelForth HEX:  
  `multicomp6809/multicomp/ROMS/6809/6809M.HEX`
- `multicomp6809` からの `nitros9_disk_manip`
- `exec09` でビルドした `m6809-run`

## フル OS ディスク再構築（ソースから）

```bash
export NITROS9DIR=~/6809/nitros9-mc09-build
export PATH="/usr/local/bin:$PATH"
cd "$NITROS9DIR"
# 初回 / クリーン後: defsfile のリンクを確保
ln -sfn mc09/defsfile level1/defsfile
ln -sfn ../mc09/defsfile level1/cmds/defsfile
ln -sfn ../mc09/defsfile level1/modules/defsfile
make dsk PORTS=mc09
# その後:
./nitros9-runtime/rebuild-runtime.sh
```

バナーを動作させるパッチは、**`wip/mc09-banner-polled`** ブランチ上でのみ有効（ポーリングされた `mc6850`、モジュール式 `mc09clock`）。

## リポジトリのレイアウト（クリーン後）

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
