# NitrOS-9 BASIC09 Starter Kit (for 6809 Sim)

PC上の6809シミュレータ環境で **NitrOS-9** および **BASIC09** を動作・開発するためのオールインワン・スターターキットです。
エミュレータ、OSビルドツリー、各種ツール、そしてすぐに起動できるランタイム環境がサブモジュールとして統合されています。

---

## ビルド環境の構築（Ubuntu/Debian系）

NitrOS-9 の OS イメージを再構築・操作するには、以下のツールが必要です。

### 1. 必要パッケージのインストール
```bash
sudo apt update
sudo apt install -y build-essential git wget curl libreadline-dev \
                    automake autoconf libtool pkg-config libfuse-dev markdown
```

### 2. LWTOOLS のインストール
NitrOS-9 のアセンブルに使用します。
```bash
wget http://lwtools.projects.l-w.ca/releases/lwtools/lwtools-4.25.tar.gz
tar xf lwtools-4.25.tar.gz
cd lwtools-4.25
make
sudo make install
```

### 3. ToolShed のインストール
ディスクイメージの操作に使用します。
```bash
git clone https://github.com/nitros9project/toolshed.git
cd toolshed
sudo make -C build/unix install
```

---

## 特徴
- **即時実行可能**: 必要なビルド成果物やスクリプトが統合されており、手軽に6809上のNitrOS-9を起動できます。
- **BASIC09対応**: BASIC09に特化したスリムなOSイメージ作成スクリプトが同梱されており、すぐにプログラミングを楽しめます。
- **オールインワン**: 6809シミュレータ、OSディスクビルドツリー、ディスクイメージ操作ツールなどがすべてサブモジュールとして一元管理されています。

---

## リポジトリ構成（サブモジュール）
本リポジトリは以下のサブモジュールから構成されています：

| ディレクトリ名 | 役割 |
| :--- | :--- |
| **`exec09`** | Arto Salmi氏の6809シミュレータを拡張したCPUシミュレータ（`m6809-run`）。 |
| **`nitros9-runtime`** | ブート用のイメージ、起動スクリプト（`run.sh`, `boot-to-shell.py`）、およびBASIC09スリムディスク作成ツール。 |
| **`nitros9-mc09-build`** | Multicomp 09向けのNitrOS-9 OSビルドツリー（バナー修正パッチ適用済み）。 |
| **`nitros9-languages`** | BASIC09 などのOS9言語環境。 |
| **`toolshed`** | OS9ディスクイメージの操作ツール群（`os9`, `decb` 等）。 |
| **`multicomp6809`** | FPGA用のソース・ROM等（ブートに必要なCamelForth ROM `6809M.HEX` やディスク操作ユーティリティを含む）。 |

---

## クイックスタート

### 1. リポジトリのクローン（サブモジュールも同時に取得）
```bash
git clone --recursive https://github.com/kyo-ta04/nitros9-basic09-starter-kit.git
cd nitros9-basic09-starter-kit
```

### 2. シミュレータ（`exec09`）のビルド
```bash
cd exec09
./configure
make
cd ..
```
*※必要に応じて `./configure --enable-readline` を指定すると、デバッガでコマンド履歴が使えるようになります。*

### 3. NitrOS-9の起動

#### インタラクティブ起動
```bash
cd nitros9-runtime
./run.sh
```
`Time ?` と表示されたら、現在時刻（例: `2026/08/15 12:00:00`）を入力して ENTER を押すとシェルが立ち上がります。

#### 自動起動（Pythonスクリプトによる時刻の自動入力）
```bash
cd nitros9-runtime
./boot-to-shell.py
```
起動が完了すると自動的に OS9 のプロンプト（`OS9:`）が表示されます。

---

## BASIC09 の起動方法
BASIC09 に特化したスリムなOSディスクイメージを使用することで、すぐにBASIC09プログラミングを始められます。

1. **BASIC09用ディスクイメージの作成・再構築**
   ```bash
   cd nitros9-runtime
   ./build-basic09-disk.sh
   ./rebuild-runtime.sh
   ```
2. **起動**
   ```bash
   ./run.sh -m 0
   ```
3. **BASIC09の実行**
   起動後のOS9プロンプトで以下を実行します：
   ```text
   OS9: mfree
   OS9: basic09
   Ready
   B:
   ```
   これで BASIC09 のプログラムを入力・実行できる状態（Ready）になります。

---

## 開発とOSのフル再構築
OSをソースコードから完全に再構築したい場合は、以下の手順を実行します：

```bash
export NITROS9DIR=~/6809/nitros9-mc09-build
export PATH="/usr/local/bin:$PATH"
cd "$NITROS9DIR"

# リンクの作成（初回またはクリーンアップ後のみ）
ln -sfn mc09/defsfile level1/defsfile
ln -sfn ../mc09/defsfile level1/cmds/defsfile
ln -sfn ../mc09/defsfile level1/modules/defsfile

# ディスクのビルド
make dsk PORTS=mc09

# ランタイムの更新
cd ../nitros9-runtime
./rebuild-runtime.sh
```

---

## ライセンス
本プロジェクトを構成する各サブモジュールは、それぞれのオープンソースライセンスに従います。
- `exec09` シミュレータ等は **GPLv2 (GNU General Public License, Version 2)** に基づきライセンスされています。詳細は [LICENSE](./LICENSE) ファイルを参照してください。
