## GPS Signal Acquisition for HLS
2025-07-07 (Published on 2025-10-10) Digital Systems Lab., AIT

For English description, please refer to [README.en.md](README.en.md).

----

### 概要

このリポジトリは，GNSS (GPS) のシリアルサーチ法による信号捕捉プログラムを，
Vitis HLS による高位合成向けに実装したもの，およびその検証環境を収録しています．

元になったプログラムは，以下のページからダウンロードできます．

https://www.rf-world.jp/bn/RFW13/RFW13DLS.shtml

実装された回路の短い説明と評価結果は，以下の論文に記載しています．

> Naoki Fujieda, Rei Yokoyama, and Takuji Ebinuma: A flexible FPGA implementation
> of GNSS signal acquisition circuit using high level synthesis, 16th International
> Workshop on Advances in Networking and Computing (WANC-16) held in conjunction with
> CANDAR '25, pp. 365-367 (11/2025).
> [DOI: [10.1109/CANDARW68385.2025.00070](https://doi.org/10.1109/CANDARW68385.2025.00070)

論文の評価を行った際のログ一式（`autobuild/log` フォルダ）のアーカイブは，
以下の URL からダウンロードできます．

https://aitech.ac.jp/~dslab/nf/gpsacq-hls/autobuild_logs.zip

### ファイル構成

このリポジトリのファイル構成は以下の通りです．

- `autobuild`: 自動高位合成/論理合成用のスクリプト一式
  - `bit`: .bit ファイルの格納先
  - `ip_dir`: IP コアの一時的格納先
  - `log`: ログの保存先
  - `autobuild_hls.rb`: Vitis HLS 用自動合成スクリプト
  - `autobuild_syn.rb`: Vivado 用自動合成スクリプト
  - `build.tcl`: Vitis HLS の高位合成手順のスクリプト
  - `build_vivado.tcl`: Vivado の高位合成手順のスクリプト
- `jupyter`: 実機検証用 Notebook 一式
  - `bits`: 実機検証用 Notebook での測定対象
    - `search[PP][FF][CC].bit`: ビットストリームファイル
    - `search[PP][FF][CC].hwh`: ハードウェア構成ファイル
  - `gps_64k.bin`: テスト用データ
  - `gpsacq_triple.ipynb`: Notebook 本体
- `codelut.cpp`: C/A コード用 LUT
- `define.hpp`: 回路・メイン関数共通の宣言
- `gpsacq_hw.cpp`: 回路化対象の関数 search の記述
- `gpsacq_main.cpp`: メイン関数の記述
- `multlut.cpp`: 乗算部分の表引き用 LUT
- `ncolut.cpp`: 搬送波 NCO の増分表引き用 LUT
- `params.hpp`: 並列度のパラメータ（autobuild_hls.rb によって自動的に作成）
- `README.md`: このファイル
- `README.en.md`: 英語版の README ファイル
- `LICENSE.txt`: ライセンス情報

`jupyter/bits` フォルダには，実機実験で使用したすべてのハードウェアオーバーレイが含まれています．
ファイル名末尾の6桁の数字は，並列度 P, F, C がそれぞれ2桁で並びます．
例えば `search160133.bit` は P = 16, F = 1, C = 33 の場合，つまり総計の並列度 528 の場合を指します．

### 実験方法

あらかじめ用意されたハードウェアオーバーレイを用いた実機実験は，以下の要領で行います．

1. `jupyter` フォルダ一式を PYNQ ボードにアップロード
2. PYNQ ボードの Jupyter Notebook で `gpsacq_triple.ipynb` を開く
3. セルを順番に実行（最後のセルはデバッグ用なので，実行しなくてもよい）

`bits` フォルダのすべての .bit ファイルを測定対象とします．
すべての測定には数時間を要します．特に，並列度 1 の場合の測定には25分程度を要します．
必要ならば，アップロードする .bit ファイルを限定するか，当該部分のコードを修正してください．

PYNQ のバージョンは v3.0.1 を想定しています．
確認はしていませんが，前後のバージョン（v3.1.2 や v2.7.0）でも動作するかもしれません．
ただし，.xsa ファイルを使う場合は，v3.0.1 以降を使わなければなりません．

### 高位合成/論理合成方法

このリポジトリに含まれるソースを高位合成・論理合成する場合は，以下の手順に従ってください．

1. Vitis HLS で新規プロジェクトを作成
  - `gpsacq_hw.cpp` の関数 `search` を合成対象とする
  - `gpsacq_main.cpp` をテストベンチとする
  - 対象デバイスは PYNQ-Z1（または PYNQ-Z2）とする
2. Vitis HLS で高位合成
  - 必要に応じて，`params.hpp` のマクロ `PAR_*` を調整
  - 必要に応じて，C Simulation
  - C Synthesis
  - Export HDL
3. Vivado で論理合成
  - 対象デバイスは PYNQ-Z1（または PYNQ-Z2）として，新規プロジェクトを作成
  - IP Repository に，1 で作成したプロジェクトのフォルダを追加
    - 自動論理合成に対応させる場合は，追加するフォルダは `autobuild/ip_dir` とする
    - この場合，当該フォルダには，Export HDL で作成された `Export.zip` の中身を展開しておく
  - Create Block Design
  - ZYNQ7 Processing System を追加し，Add Block Automation
  - ZYNQ7 Processing System をダブルクリックし，PS/PL Configuration から HP ポートを1つ有効化
  - search を追加し，Add Connection Automation
  - AXI Direct Memory Access を追加して，ダブルクリック
    - Enable Scatter Gather Engine と Enable Write Channel のチェックを外す
    - Width of Buffer Length Register を 15（以上）に設定
    - Read Channel の Stream Data Width を 8 に設定
  - Add Connection Automation
  - AXI Direct Memory Access の M_AXIS_MM2S を Search の sig に接続
  - あとは通常通りの手順で Generate Bitstream まで行う
  - Export Hardware → Include Bitstream → .xsa ファイルを適当な名前で保存
    - または，.bit ファイルおよび .hwh ファイルを抽出して，名前を変更
  
### 自動高位合成/論理合成 の方法

`autobuild` フォルダには，並列度のパラメータを変更しながら，
繰り返し高位合成・論理合成を行うための Ruby スクリプトが含まれています．
スクリプトは Windows 環境のみに対応しています．
RubyInstaller などを用いて，Windows 版の Ruby をインストールしておいてください．
スクリプトは，`autobuild` フォルダのあるパスを作業ディレクトリとした状態で実行してください．

自動高位合成/論理合成を行うには，あらかじめ「高位合成/論理合成方法」の手順に従い，
これらのプロジェクトをリポジトリ直下のフォルダとして置いておく必要があります．
標準のフォルダ名は，高位合成が `hls_proj`，論理合成が `fpga_proj` です．

自動高位合成のスクリプトは，`autobuild_hls.rb` です．
あらかじめ，スクリプト7行目の `HLS_DIR` から11行目の `BASE_DIR` までの定数を，
各自の環境に合わせて修正しておいてください．
P, F のそれぞれの組に対して，C を増やしながら高位合成を繰り返します．
ただし，途中で見積もられた LUT の個数が 50,000 を超えた場合，
PYNQ ボードへの実装は困難であるとみなし，より大きな C に対する合成をスキップします．

自動論理合成のスクリプトは，`autobuild_syn.rb` です．
あらかじめ，スクリプト7行目の `VIVADO_DIR` から11行目の `BASE_DIR` までの定数を，
各自の環境に合わせて修正しておいてください．
論理合成対象は，`autobuild/log` フォルダにある高位合成ログにより決定します．
メインループの II が 1 でないもの，見積もられた LUT の個数が 30,000 より大きいものは，
論理合成をスキップします．

### 著作権

本リポジトリのすべての C++, Ruby ソースコードは，
[藤枝 直輝](https://aitech.ac.jp/~dslab/nf/) により開発され，
愛知工業大学 ディジタルシステム研究室が著作権を保有します．ライセンスは New BSD です．
コードの一部には，ディジタルシステム研究室の卒業生である 横山 玲 の貢献を含みます．

Copyright (C) 2024-2025 Digital Systems Laboratory. All rights reserved.

また `gpsacq_hw.cpp` および `gpsacq_main.cpp` のベースとなったプログラム `gpsacq.c` は，
海老沼 拓史 により開発され，以下の URL からダウンロードできるものです．

https://www.rf-world.jp/bn/RFW13/RFW13DLS.shtml

`gpsacq.c` の二次利用，および派生したプログラムを New BSD ライセンスで公開することについては，
原著者の個別の許諾を得ております．
`gpsacq.c` の二次利用については，原著者に問い合わせてください．
また，`gpsacq.c` に関する免責事項は，上記のダウンロードページからご確認ください．

gpsacq.c: Copyright (C) 2011 Takuji Ebinuma

ライセンスに関する詳細は LICENSE.txt を確認してください．