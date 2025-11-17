# BMI270 Driver Development Examples

このディレクトリには、BMI270 SPIドライバの開発過程を段階的に示すサンプルが含まれています。

**注意**: これらは開発過程を示す教育的なサンプルです。実際のアプリケーションでは、[`examples/basic_*`](../)配下のシンプルなサンプルを参照してください。

## 開発ステージ

### Stage 1: SPI Basic Communication - SPI基本通信
- ディレクトリ: `stage1_spi_basic/`
- 目的: BMI270とのSPI通信を確立し、CHIP_IDを読み取る
- 学習内容:
  - ESP-IDF SPI API基礎
  - BMI270 SPI通信プロトコル（ダミーバイト）
  - 基本的なレジスタ読み取り

### Stage 2: Sensor Initialization - センサー初期化
- ディレクトリ: `stage2_init/`
- 目的: BMI270を完全に初期化し、使用可能な状態にする
- 学習内容:
  - コンフィグファイル（8KB）のアップロード
  - 初期化シーケンス（パワーオン→コンフィグ→センサー有効化）
  - エラーハンドリング

### Stage 3: Polling Data Read - ポーリングデータ読み取り
- ディレクトリ: `stage3_polling/`
- 目的: ポーリングでジャイロ・加速度データを定期的に読み取る
- 学習内容:
  - DATA_RDYステータス確認
  - データレジスタ読み取り
  - 生データから物理値への変換

### Stage 4: Interrupt-Driven Read - 割り込み駆動読み取り
- ディレクトリ: `stage4_interrupt/`
- 目的: DATA_RDY割り込みでデータを効率的に読み取る
- 学習内容:
  - INT1ピン設定
  - GPIO割り込みハンドラ
  - FreeRTOSセマフォによる同期

### Stage 5: FIFO Read - FIFO読み取り
- ディレクトリ: `stage5_fifo/`
- 目的: FIFOバッファを使用した高速・効率的なデータ取得
- サブステップ:
  - **Step 1**: 基本的なFIFO読み取り（1フレーム）
  - **Step 2**: 複数フレーム読み取りと平均化
  - **Step 3**: FIFOフラッシュとエラーハンドリング
  - **Step 4**: ウォーターマーク割り込み（本サンプル）

## ビルド方法

各ステージは独立してビルド可能です：

```bash
# 環境セットアップ
source ~/esp/esp-idf/export.sh

# Stage 1をビルド
cd stage1_spi_basic
idf.py set-target esp32s3
idf.py build
idf.py flash monitor

# Stage 5 Step 4をビルド
cd stage5_fifo/step4_watermark_int
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## 推奨学習順序

1. **初心者**: Stage 1 → Stage 2 → Stage 3
2. **中級者**: Stage 3 → Stage 4 → Stage 5 Step 1-2
3. **上級者**: Stage 5 Step 4（FIFO割り込み）を直接参照

## 実用サンプル

実際のアプリケーション開発には、以下のシンプルなサンプルを推奨：

- [`examples/basic_polling/`](../basic_polling/) - ポーリング読み取り（最もシンプル）
- [`examples/basic_interrupt/`](../basic_interrupt/) - 割り込み読み取り（効率的）
- [`examples/basic_fifo/`](../basic_fifo/) - FIFO読み取り（高速・低CPU）

## 参考資料

- [BMI270データシート](https://www.bosch-sensortec.com/products/motion-sensors/imus/bmi270/)
- [M5StampFly仕様書](../../docs/M5StampFly_spec_ja.md)
- [API仕様書](../../docs/API.md)
