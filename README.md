# BMI270 IMU Driver for ESP32-S3 (M5StampFly)

ESP-IDF v5.4.1ベースのBMI270 6軸IMUドライバ実装プロジェクト

## プロジェクト概要

M5StampFly（ESP32-S3搭載）向けのBMI270 IMUドライバを段階的に開発します。
このコンポーネントは他のESP-IDFプロジェクトの`components/`フォルダにコピーするだけで使用可能です。

### ハードウェア仕様

- **MCU**: ESP32-S3 (M5StampS3)
- **IMU**: BMI270 (Bosch Sensortec製 6軸IMU)
- **通信**: SPI (10 MHz, Mode 0)
- **共有SPIバス**: PMW3901光学フローセンサと共有

### ピン接続

| 信号 | ESP32-S3 GPIO | BMI270ピン |
|------|---------------|------------|
| MOSI | GPIO14 | SDx (SDI) |
| MISO | GPIO43 | SDO |
| SCK  | GPIO44 | SCx |
| CS   | GPIO46 | CSB |

**注意**: GPIO12（PMW3901 CS）を適切に管理する必要があります

---

## 開発ステージ

### Stage 1: SPI基本通信 ✅ **完了**

**目標**: BMI270との安定したSPI通信確立

**達成内容**:
- ✅ ESP-IDF SPI Master Driver統合
- ✅ 3バイトReadトランザクション実装
- ✅ CHIP_ID検証 (0x24)
- ✅ 100%通信成功率達成
- ✅ 共有SPIバス対応（PMW3901 CS管理）
- ✅ 遅延最適化（1000µs採用）

**重要な成果**:
- PMW3901 CSの浮き状態が50%失敗の原因と判明・解決
- 50µs〜5000µs全てで100%成功を確認
- 実機テストによる最適遅延値決定

詳細: [examples/stage1_spi_basic/README.md](examples/stage1_spi_basic/README.md)

### Stage 2: BMI270初期化シーケンス ✅ **完了**

**目標**: BMI270の完全な初期化

**達成内容**:
- ✅ ソフトリセット実装
- ✅ 8KBコンフィグファイルアップロード（32バイトバースト）
- ✅ INTERNAL_STATUSポーリング（最大20msタイムアウト）
- ✅ 初期化完了確認（message = 0x01検証）
- ✅ ACC/GYR有効化（PWR_CTRL = 0x06）
- ✅ 動的タイミング制御（通常モード2µs切替）

**重要な成果**:
- 8192バイトBosch公式コンフィグ統合
- 初期化シーケンス完全自動化
- 詳細なログとエラー処理
- ビルドテスト成功

詳細: [examples/stage2_init/README.md](examples/stage2_init/README.md)

### Stage 3: ポーリング読み取り 📋 **予定**

**目標**: 基本的なセンサーデータ読み取り

**実装予定**:
- 加速度・角速度データ読み取り
- データフォーマット変換
- サンプリングレート設定

### Stage 4: 割り込み読み取り 📋 **予定**

**目標**: 効率的なデータ取得

**実装予定**:
- データレディ割り込み設定
- 割り込みハンドラ実装
- イベント駆動型データ読み取り

### Stage 5: FIFO高速読み取り 📋 **予定**

**目標**: 高速・大量データ取得

**実装予定**:
- FIFOバッファ設定
- バーストリード実装
- 高周波数サンプリング対応

---

## ディレクトリ構造

```
stampfly_imu/
├── README.md                    # このファイル
├── CMakeLists.txt              # コンポーネント定義
├── .gitignore                  # Git除外設定
│
├── include/                    # 公開ヘッダー
│   ├── bmi270_defs.h          # レジスタ定義・定数
│   ├── bmi270_types.h         # 型定義・構造体
│   └── bmi270_spi.h           # SPI通信API
│
├── src/                        # 実装ファイル
│   └── bmi270_spi.c           # SPI通信層
│
├── docs/                       # ドキュメント
│   ├── M5StamFly_spec_ja.md   # ハードウェア仕様
│   ├── bmi270_doc_ja.md       # BMI270実装ガイド
│   ├── bmi270_configfile.c    # Bosch公式コンフィグ
│   └── esp_idf_bmi270_spi_guide.md  # ESP-IDF SPI実装ガイド
│
└── examples/                   # テストプログラム
    └── stage1_spi_basic/      # Stage 1: SPI基本通信
        ├── main/
        │   └── main.c
        ├── CMakeLists.txt
        ├── sdkconfig.defaults
        └── README.md
```

---

## 使用方法

### 1. コンポーネントとして使用

他のESP-IDFプロジェクトで使用する場合：

```bash
cd <your-project>/components/
cp -r /path/to/stampfly_imu .
cd ..
idf.py build
```

### 2. サンプルプログラムのビルド＆実行

```bash
# 環境セットアップ
source ~/esp/esp-idf/export.sh

# Stage 1サンプルをビルド
cd examples/stage1_spi_basic
idf.py set-target esp32s3
idf.py build

# フラッシュ＆モニター
idf.py flash monitor
```

---

## API概要（Stage 1時点）

### SPI初期化

```c
#include "bmi270_spi.h"

bmi270_dev_t dev = {0};
bmi270_config_t config = {
    .gpio_mosi = 14,
    .gpio_miso = 43,
    .gpio_sclk = 44,
    .gpio_cs = 46,
    .spi_clock_hz = 10000000,
    .spi_host = SPI2_HOST,
    .gpio_other_cs = 12,  // PMW3901 CS（共有SPI対策）
};

esp_err_t ret = bmi270_spi_init(&dev, &config);
```

### レジスタ読み書き

```c
// 読み取り
uint8_t chip_id;
bmi270_read_register(&dev, BMI270_REG_CHIP_ID, &chip_id);

// 書き込み
bmi270_write_register(&dev, BMI270_REG_PWR_CTRL, 0x0E);
```

### バースト読み書き

```c
// バースト読み取り
uint8_t data[12];
bmi270_read_burst(&dev, BMI270_REG_ACC_X_LSB, data, 12);

// バースト書き込み
uint8_t config_data[8192];
bmi270_write_burst(&dev, BMI270_REG_INIT_DATA, config_data, 8192);
```

---

## 技術的ハイライト

### BMI270 SPI通信プロトコル

#### 3バイトRead（重要）

```
TX: [CMD: 0x80|addr] [Dummy] [Dummy]
RX: [Echo]           [Dummy] [DATA]  ← 3バイト目が有効データ
```

#### 2バイトWrite

```
TX: [CMD: 0x00|addr] [DATA]
```

### 動的タイミング制御

- **低電力モード**: 1000µs遅延（初期化前）
- **通常モード**: 2µs遅延（初期化後）

### 共有SPIバス管理

M5StampFlyではBMI270とPMW3901が同じSPIバスを共有：
```c
// PMW3901 CSを明示的に非アクティブ化
gpio_set_level(12, 1);  // HIGH = 非アクティブ
```

---

## 開発環境

- **ESP-IDF**: v5.4.1
- **ツールチェーン**: Xtensa ESP32-S3
- **ターゲット**: ESP32-S3
- **ビルドシステム**: CMake + Ninja

---

## トラブルシューティング

### 通信が不安定（成功率 < 100%）

**最も可能性が高い原因**: 共有SPIバスの他デバイスCSが浮いている

**解決策**:
1. 全デバイスのCSを確認
2. 未使用デバイスのCSをHIGHに固定
3. M5StampFlyの場合: PMW3901のCS（GPIO12）を必ず管理

詳細: [examples/stage1_spi_basic/README.md#開発経過と重要な知見](examples/stage1_spi_basic/README.md#開発経過と重要な知見)

### CHIP_IDが読めない

**確認事項**:
1. ピン接続の再確認
2. 3.3V電源供給確認
3. SPI Mode 0設定確認
4. ダミーリードでSPIモード起動確認

---

## ライセンス

このプロジェクトのライセンスは未定です。

---

## 参考資料

- [BMI270 Datasheet](https://www.bosch-sensortec.com/products/motion-sensors/imus/bmi270/)
- [ESP-IDF SPI Master Driver](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-reference/peripherals/spi_master.html)
- [M5StampFly Documentation](docs/M5StamFly_spec_ja.md)

---

## 進捗状況

| Stage | 状態 | 完了率 |
|-------|------|--------|
| Stage 1: SPI基本通信 | ✅ 完了 | 100% |
| Stage 2: 初期化シーケンス | ✅ 完了 | 100% |
| Stage 3: ポーリング読み取り | 📋 予定 | 0% |
| Stage 4: 割り込み読み取り | 📋 予定 | 0% |
| Stage 5: FIFO高速読み取り | 📋 予定 | 0% |

**全体進捗**: 40% (2/5ステージ完了)

---

## 開発者

このプロジェクトはClaude Code (Anthropic)との協働により開発されています。

**Generated with [Claude Code](https://claude.com/claude-code)**
