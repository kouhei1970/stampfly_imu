# Stage 5: BMI270 FIFO High-Speed Data Reading

## 目的

BMI270のFIFO機能を使用して高速・効率的にセンサーデータを読み取ります：

- FIFOバッファの設定（センサー有効化、ヘッダーモード、ウォーターマーク）
- ウォーターマーク割り込みを使用したバッチ読み取り
- FIFOフレームのパース（加速度+ジャイロの複合フレーム）
- SPI通信回数の大幅削減による高効率動作

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
| INT1 | GPIO11 | INT1 |

## センサー設定

| パラメータ | 設定値 | 説明 |
|----------|--------|------|
| 加速度レンジ | ±4g | 高精度測定 |
| ジャイロレンジ | ±1000 °/s | 一般的な動作範囲 |
| 加速度ODR | 100 Hz | サンプリング周波数 |
| ジャイロODR | 200 Hz | サンプリング周波数 |
| FIFOモード | ヘッダーモード | フレーム識別用 |
| FIFOウォーターマーク | 512 bytes | 約39フレーム |

## FIFO設定詳細

### FIFOフレーム構成

このサンプルでは加速度とジャイロの両方を有効化するため、**複合フレーム**が使用されます：

```
[Header: 0x8C] [GYR_X_L] [GYR_X_H] [GYR_Y_L] [GYR_Y_H] [GYR_Z_L] [GYR_Z_H]
               [ACC_X_L] [ACC_X_H] [ACC_Y_L] [ACC_Y_H] [ACC_Z_L] [ACC_Z_H]
```

**重要**: BMI270のFIFOでは、**ジャイロが先、加速度が後**の順序で格納されます。

- **フレームサイズ**: 13バイト（1ヘッダー + 6ジャイロ + 6加速度）
- **1フレームのデータ量**: 両センサーの3軸データ

### ウォーターマーク設定

- **設定値**: 512バイト
- **フレーム数**: 512 ÷ 13 ≈ 39フレーム
- **データ取得頻度**: 200Hz ÷ 39 ≈ 5.1Hz（約196msごと）
- **SPI通信回数**: 約**15回/秒**（vs. Stage 4の200回/秒）

## 性能比較

| 項目 | Stage 3<br>(Polling) | Stage 4<br>(Interrupt) | Stage 5<br>(FIFO) |
|------|---------------------|----------------------|------------------|
| CPU使用率 | 高 | 低 | 最低 |
| SPI通信回数/秒 | 200回 | 200回 | **15回** |
| 割り込み回数/秒 | - | 200回 | **15回** |
| データ遅延 | 最大100ms | < 10µs | 最大196ms |
| データ損失リスク | 低 | 低 | 最低（FIFO保護） |
| 適用用途 | 低速アプリ | リアルタイム | 高速ロギング |

## ビルド＆実行

### 1. 環境セットアップ

```bash
source ~/esp/esp-idf/export.sh
```

### 2. ターゲット設定（初回のみ）

```bash
cd examples/stage5_fifo
idf.py set-target esp32s3
```

### 3. ビルド

```bash
idf.py build
```

### 4. フラッシュ＆モニター

```bash
idf.py flash monitor
```

終了: `Ctrl+]`

## 出力設定

デフォルトでは、物理値（g、°/s、°C）のみを出力します。RAW値（LSB）を表示したい場合は、[main.c:35](main/main.c#L35)を修正してください:

```c
#define OUTPUT_RAW_VALUES   1  // Set to 1 to output raw sensor values (LSB)
```

## 期待される出力

### 初期化ログ

```
I (xxx) BMI270_STAGE5: ========================================
I (xxx) BMI270_STAGE5:  BMI270 Stage 5: FIFO Data Reading
I (xxx) BMI270_STAGE5: ========================================

I (xxx) BMI270_STAGE5: Step 1: Initializing SPI...
I (xxx) BMI270_SPI: SPI bus initialized on host 1
I (xxx) BMI270_SPI: BMI270 SPI device added successfully
I (xxx) BMI270_STAGE5: ✓ SPI initialized successfully

...

I (xxx) BMI270_STAGE5: Step 7: Configuring FIFO...
I (xxx) BMI270_FIFO: FIFO configured: acc=1, gyr=1, header=1, stop_on_full=1, watermark=512
I (xxx) BMI270_STAGE5: ✓ FIFO configured (watermark: 512 bytes)

I (xxx) BMI270_STAGE5: ========================================
I (xxx) BMI270_STAGE5:  FIFO Data Stream (Teleplot format)
I (xxx) BMI270_STAGE5: ========================================
```

### デフォルト出力（物理値のみ）

Teleplot形式で8チャンネルのデータを出力（バッチ読み取り、約5Hz）:

```
>acc_x:-0.0299
>acc_y:0.0156
>acc_z:1.0000
>gyr_x:0.366
>gyr_y:-0.244
>gyr_z:0.091
>temp:25.00
>fifo_count:39
```

### RAW値有効時 (OUTPUT_RAW_VALUES=1)

15チャンネル（RAW値を含む）:

```
>acc_raw_x:-245
>acc_raw_y:128
>acc_raw_z:8192
>acc_x:-0.0299
>acc_y:0.0156
>acc_z:1.0000
>gyr_raw_x:12
>gyr_raw_y:-8
>gyr_raw_z:3
>gyr_x:0.366
>gyr_y:-0.244
>gyr_z:0.091
>temp_raw:1024
>temp:25.00
>fifo_count:39
```

ステータスメッセージ（バッチ処理ごと）:

```
I (1234) BMI270_STAGE5: FIFO watermark reached: 39 frames read
I (1430) BMI270_STAGE5: FIFO watermark reached: 39 frames read
```

## 成功基準

- [x] SPI通信成功
- [x] BMI270初期化成功
- [x] センサー設定成功（レンジ、ODR）
- [x] FIFO設定成功（ヘッダーモード、ウォーターマーク）
- [x] INT1ピン設定成功
- [x] ウォーターマーク割り込み動作確認
- [x] FIFOデータ読み取り成功
- [x] フレームパース成功（加速度+ジャイロ複合フレーム）
- [x] 連続データ読み取り（安定）
- [x] SPI通信回数削減確認（約15回/秒）

## 重要な技術ポイント

### 1. FIFOヘッダーモード

ヘッダーモードでは、各フレームの先頭1バイトがフレーム種別を示します：

- `0x84`: 加速度のみ（7バイトフレーム）
- `0x88`: ジャイロのみ（7バイトフレーム）
- `0x8C`: 加速度+ジャイロ（13バイトフレーム） ← このサンプルで使用

### 2. ウォーターマーク割り込み

FIFOに指定バイト数以上のデータが蓄積されると割り込みが発生します：

```c
// 512バイト ÷ 13バイト/フレーム ≈ 39フレーム
// 200Hz ÷ 39フレーム ≈ 5.1Hz（約196msごとに割り込み）
```

### 3. バースト読み取り

FIFOデータは`FIFO_DATA`レジスタ（0x26）からバースト読み取りします：

```c
uint16_t fifo_length;
bmi270_get_fifo_length(&dev, &fifo_length);  // FIFOに蓄積されたバイト数

uint8_t fifo_buffer[1024];
bmi270_read_fifo_data(&dev, fifo_buffer, fifo_length);  // 一括読み取り
```

### 4. フレームパース

読み取ったFIFOバッファから個別のフレームを抽出：

```c
const uint8_t *ptr = fifo_buffer;
uint16_t remaining = fifo_length;

while (remaining > 0) {
    bmi270_fifo_frame_t frame;
    esp_err_t ret = bmi270_parse_fifo_frame(&ptr, &remaining, &frame);

    if (ret == ESP_OK && frame.type == BMI270_FIFO_FRAME_ACC_GYR) {
        // frame.acc.x, frame.acc.y, frame.acc.z
        // frame.gyr.x, frame.gyr.y, frame.gyr.z
    }
}
```

### 5. Stop-on-Full モード

このサンプルでは`stop_on_full = true`を設定：

- **true**: FIFOが満杯になると新しいデータの書き込みを停止（データ損失なし）
- **false**: FIFOが満杯になると古いデータを上書き（最新データ優先）

## トラブルシューティング

### ウォーターマーク割り込みが発生しない

**対策**:
1. INT1ピン設定を確認（GPIO11、プルダウン有効）
2. ウォーターマーク値を確認（0より大きい値）
3. FIFO_CONFIG_1を確認（センサーとヘッダーが有効化されているか）
4. INT_MAP_FEAT レジスタを確認（bit 1が設定されているか）

### FIFOが空のまま

**対策**:
1. PWR_CTRLを確認（加速度とジャイロが有効化されているか）
2. FIFO_CONFIG_1を確認（ACC_EN、GYR_ENが設定されているか）
3. ODR設定を確認（ACC_CONF、GYR_CONFレジスタ）

### フレームパースエラー

**対策**:
1. ヘッダーモードが有効か確認（FIFO_CONFIG_1 bit 4）
2. FIFOバッファのサイズが十分か確認（≥ フレームサイズ）
3. FIFO_LENGTHが正しく読み取れているか確認

### データが古い（遅延が大きい）

FIFOは効率重視のため、リアルタイム性は低下します：

- **解決策1**: ウォーターマークを小さく設定（例: 256バイト → 約98ms）
- **解決策2**: リアルタイム性が必要な場合はStage 4（割り込み）を使用

## 次のステップ

Stage 5が成功したら、以下の応用に進むことができます：

- **モーション検出**: Any-motion、No-motion、Step counter割り込み
- **低消費電力モード**: Advanced Power Save機能の活用
- **データロギング**: SDカードへの高速記録
- **フィルタリング**: ESP32側でのデジタルフィルタ処理

## 参考ドキュメント

- [docs/bmi270_doc_ja.md](../../docs/bmi270_doc_ja.md) - BMI270実装ガイド
  - 4.8節: FIFO機能
- [include/bmi270_fifo.h](../../include/bmi270_fifo.h) - FIFO API仕様
- [src/bmi270_fifo.c](../../src/bmi270_fifo.c) - FIFO実装
