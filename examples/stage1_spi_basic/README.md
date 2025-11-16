# Stage 1: BMI270 SPI Basic Communication Test

## 目的

ESP-IDF SPI Masterドライバを使用してBMI270との基本的なSPI通信を確立し、以下を確認します：

- SPI通信の初期化
- 3バイトトランザクション実装の正確性
- CHIP_ID読み取り（期待値: 0x24）
- 通信の安定性

## ハードウェア

- **MCU**: ESP32-S3 (M5StampS3)
- **センサー**: BMI270 6軸IMU
- **通信**: SPI (10 MHz, Mode 0)

### ピン接続

| 信号 | ESP32-S3 GPIO | BMI270ピン |
|------|---------------|------------|
| MOSI | GPIO14 | SDx (SDI) |
| MISO | GPIO43 | SDO |
| SCK  | GPIO44 | SCx |
| CS   | GPIO46 | CSB |

## ビルド＆実行

### 1. 環境セットアップ

```bash
source ~/esp/esp-idf/export.sh
```

### 2. ビルド

```bash
cd examples/stage1_spi_basic
idf.py build
```

### 3. フラッシュ＆モニター

```bash
idf.py flash monitor
```

終了: `Ctrl+]`

## 期待される出力

```
I (xxx) BMI270_TEST: ========================================
I (xxx) BMI270_TEST:  BMI270 Stage 1: SPI Basic Communication
I (xxx) BMI270_TEST: ========================================

I (xxx) BMI270_TEST: Step 1: Initializing SPI...
I (xxx) BMI270_SPI: SPI bus initialized on host 1
I (xxx) BMI270_SPI: BMI270 SPI device added successfully
I (xxx) BMI270_SPI: GPIO - MOSI:14 MISO:43 SCLK:44 CS:46
I (xxx) BMI270_SPI: SPI Clock: 10000000 Hz
I (xxx) BMI270_TEST: ✓ SPI initialized successfully

I (xxx) BMI270_TEST: Step 2: Activating SPI mode...
I (xxx) BMI270_TEST: Performing dummy read to activate SPI mode...
I (xxx) BMI270_TEST: SPI mode activated

I (xxx) BMI270_TEST: Step 3: Testing CHIP_ID...
I (xxx) BMI270_TEST: Reading CHIP_ID...
I (xxx) BMI270_TEST: CHIP_ID = 0x24 (expected: 0x24)
I (xxx) BMI270_TEST: ✓ CHIP_ID verification SUCCESS

I (xxx) BMI270_TEST: Step 4: Testing communication stability...
I (xxx) BMI270_TEST: Testing communication stability (100 iterations)...
I (xxx) BMI270_TEST: Communication test results:
I (xxx) BMI270_TEST:   Success: 100/100 (100.0%)
I (xxx) BMI270_TEST:   Failed:  0/100
I (xxx) BMI270_TEST: ✓ Communication stability test PASSED

I (xxx) BMI270_TEST: ========================================
I (xxx) BMI270_TEST:  ✓ ALL TESTS PASSED
I (xxx) BMI270_TEST: ========================================
I (xxx) BMI270_TEST: Stage 1 completed successfully!
I (xxx) BMI270_TEST: BMI270 SPI communication is working correctly.
I (xxx) BMI270_TEST: Next step: Stage 2 - Initialization sequence
```

## 成功基準

- [x] CHIP_ID = 0x24 を読み取れる
- [x] 100回連続読み取りで100%成功
- [x] SPIエラーが発生しない

## トラブルシューティング

### CHIP_IDが0x00または0xFFを返す

**原因**: 配線ミスまたはSPI設定エラー

**対策**:
1. ピン接続を再確認
2. プルアップ/プルダウン抵抗の確認
3. 3.3V電源電圧の確認
4. オシロスコープで信号確認

### CHIP_IDが異なる値を返す

**原因**: BMI270が別のモードで動作中

**対策**:
1. 電源を再投入
2. ダミーリードを追加実行
3. SPIモード設定を確認（Mode 0またはMode 3）

### 通信が不安定（成功率 < 100%）

**原因**: クロック周波数が高すぎる、またはノイズ

**対策**:
1. SPI クロックを下げる（5 MHz程度）
2. 配線を短くする
3. デカップリングコンデンサを追加

## 次のステップ

Stage 1が成功したら、次は **Stage 2: BMI270初期化シーケンス** に進みます。

Stage 2では以下を実装します：
- 電源投入シーケンス
- 8KBコンフィグファイルのロード
- INTERNAL_STATUSポーリング
- 初期化完了確認

## 参考ドキュメント

- [docs/esp_idf_bmi270_spi_guide.md](../../docs/esp_idf_bmi270_spi_guide.md) - ESP-IDF SPI実装ガイド
- [docs/bmi270_doc_ja.md](../../docs/bmi270_doc_ja.md) - BMI270実装ガイド
- [docs/M5StamFly_spec_ja.md](../../docs/M5StamFly_spec_ja.md) - ハードウェア仕様
