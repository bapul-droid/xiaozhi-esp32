# MINJI

> **README AS REALME** — yang tertulis di sini harus mencerminkan perilaku Minji yang benar-benar terlihat di hardware, bukan sekadar kemampuan yang secara teori mungkin didukung upstream.

Minji adalah turunan/custom build dari proyek [XiaoZhi ESP32](https://github.com/78/xiaozhi-esp32) yang dikembangkan menjadi perangkat AI fisik dengan wajah sendiri, hardware tetap, layanan lokal Genius, diagnostik perangkat, dukungan baterai, serta ekspansi multimedia.

README ini sengaja berfokus pada **hardware dan perilaku Minji yang benar-benar dipakai**. Dokumentasi upstream XiaoZhi tetap menjadi referensi untuk protokol dan komponen dasar yang belum didokumentasikan ulang di sini.

---

## Status Legend

| Status | Arti |
|---|---|
| ✅ **CONFIRMED** | Sudah diuji langsung pada hardware Minji dan bekerja. |
| 📋 **MANUFACTURER / SELLER SPEC** | Informasi dari nama/listing produk; belum otomatis dianggap terbukti di Minji. |
| 🔎 **UNDER INVESTIGATION** | Sedang diukur/ditelusuri. Jangan dijadikan dasar wiring final. |
| 🧪 **EXPERIMENTAL** | Implementasi sudah ada atau sedang diuji, tetapi belum dianggap stabil/final. |
| 🗺️ **PLANNED** | Masuk roadmap, belum tersedia sebagai fitur siap pakai. |

**Aturan dokumentasi:** kata *supported* tidak dipakai sebagai sinonim *working*. Jika sesuatu ditulis **CONFIRMED**, berarti Minji benar-benar pernah menjalankannya.

---

# 1. Minji Hardware Platform

## Main MCU

| Item | Minji |
|---|---|
| MCU | ESP32-S3 |
| Flash | 16 MB |
| PSRAM | 8 MB Octal |
| Board class | N16R8 |
| Display | TFT/LCD 1.8 inch, 128×160 |
| Audio | I2S microphone + internal speaker path |
| Connectivity | Wi-Fi; Bluetooth expansion terpisah sedang dikembangkan |

Status: ✅ **CONFIRMED**

## Minji Expansion Board

Nama yang digunakan dalam proyek:

**ESP32-S3 Smart Expansion Board V1.7**

Nama lain yang umum ditemukan pada listing marketplace:

> **ESP32-S3 Smart Expansion Board V1.7 | Integrated I2S Audio Amplifier with TFT Display Development Base Board N16R8 Applicable**

Board ini adalah bagian penting dari hardware Minji, bukan sekadar adaptor mekanis. Pada unit yang digunakan proyek Minji, board ini terlibat pada integrasi display/audio, koneksi USB/UART, dan operasi baterai.

### Confirmed on Minji

- ✅ ESP32-S3 N16R8 dapat berjalan pada expansion board ini.
- ✅ LCD 1.8 inch 128×160 bekerja dengan konfigurasi firmware Minji.
- ✅ Microphone dan internal speaker bekerja.
- ✅ Minji dapat hidup tanpa kabel USB menggunakan baterai melalui expansion board.
- ✅ Charging battery melalui expansion board telah terlihat bekerja; LED charging menyala saat pengisian.
- ✅ Terdapat dua jalur USB yang menunjukkan perilaku berbeda pada Windows dan pada Black Box Minji.

### Manufacturer / Seller Specification

- 📋 Disebut sebagai **Smart Expansion Board V1.7** untuk ESP32-S3 N16R8.
- 📋 Listing menyebut integrasi **I2S audio amplifier** dan **TFT display development base board**.

Informasi seller di atas dicatat untuk membantu pencarian spare part. Detail elektrik tetap harus diverifikasi sebelum dipakai sebagai fakta wiring.

### Under Investigation

- 🔎 Tegangan rail yang tersedia saat **battery-only**: apakah tersedia rail 5 V stabil untuk beban eksternal.
- 🔎 Kapasitas arus rail tersebut untuk memberi daya ESP32-WROOM tambahan.
- 🔎 Topologi charging/power-path lengkap expansion board.
- 🔎 Titik terbaik untuk mengambil supply WROOM tanpa backfeed saat salah satu board terhubung USB.

**Jangan menghubungkan WROOM ke rail expansion secara permanen sebelum bagian ini selesai diuji.**

---

# 2. Known Minji Hardware Behavior

Bagian ini berisi perilaku yang benar-benar terlihat saat pengujian. Ini sengaja dipisahkan dari spesifikasi seller.

## USB-C / Serial Behavior

### Port kiri — Native USB ESP32-S3

Status: ✅ **CONFIRMED**

- Windows mendeteksi sebagai **COM8** pada unit pengujian.
- Saat boot/reset melalui jalur ini, Black Box Minji mencatat sumber reset sebagai **USB**.
- Digunakan sebagai native USB ESP32-S3.

### Port kanan — CH340C / UART

Status: ✅ **CONFIRMED**

- Windows mendeteksi sebagai **COM10** pada unit pengujian.
- Jalur ini menggunakan CH340C/UART bridge.
- Dalam Black Box Minji, boot melalui jalur ini terlihat sebagai **POWERON**, bukan USB native.

### Battery / Powerbank

Status: ✅ **CONFIRMED**

- Minji dapat tetap hidup tanpa kabel USB menggunakan baterai melalui expansion board.
- Power dari powerbank/battery dicatat Black Box sebagai **POWERON**.

### Safety rule during testing

> Hindari memakai dua sumber USB sekaligus selama eksperimen power. Saat pengujian battery/charging, gunakan satu sumber daya pada satu waktu sampai topologi power final benar-benar diketahui.

---

# 3. Display — Known Working Configuration

Status: ✅ **CONFIRMED**

Minji menggunakan LCD **1.8 inch 128×160**. Firmware generik yang berhasil boot belum tentu menghasilkan gambar pada panel ini.

## Known failure: LCD putih

Gejala yang pernah terjadi:

- ESP32-S3 boot.
- Audio/Wi-Fi dapat bekerja.
- Backlight LCD menyala.
- Tampilan tetap putih.

Artinya perangkat **belum tentu rusak**. Pada Minji, kasus ini pernah berasal dari ketidakcocokan konfigurasi display/board.

## Known Minji LCD pin mapping

| Signal | GPIO |
|---|---:|
| LCD SDA | 47 |
| LCD RES | 45 |
| LCD DC | 40 |
| LCD CS | 41 |
| LCD BLK | 42 |

Board configuration yang menjadi baseline Minji:

```text
CONFIG_BOARD_TYPE_BREAD_COMPACT_WIFI_LCD=y
```

> Jika hardware yang terlihat sama menghasilkan LCD putih, jangan langsung mengganti panel. Cocokkan board variant, driver display, pin mapping, orientation, dan backlight configuration terlebih dahulu.

---

# 4. Audio Hardware

Status: ✅ **CONFIRMED** untuk audio internal.

## I2S microphone pins

| Signal | GPIO |
|---|---:|
| MIC WS | 4 |
| MIC SCK | 5 |
| MIC DIN | 6 |

## Internal speaker I2S pins

| Signal | GPIO |
|---|---:|
| SPK DOUT | 7 |
| SPK BCLK | 15 |
| SPK LRCK | 16 |

Minji dapat berbicara melalui speaker internal dan menerima suara melalui microphone internal.

### Known audio issue / investigation history

- Audio noise pernah menjadi issue investigasi pada hardware Minji.
- Media playback dan voice interaction harus diperlakukan sebagai pipeline yang berbeda agar perintah suara tetap dapat diterima ketika media aktif.

---

# 5. Battery & Power Architecture

## Current confirmed state

Status: ✅ **CONFIRMED**

```text
Battery
   │
   ▼
Minji Expansion Board V1.7
   │
   ▼
ESP32-S3 Minji
```

Minji sudah dapat beroperasi battery-only.

## Target dual-ESP architecture

Status: 🔎 **UNDER INVESTIGATION**

Target untuk Minji + Bluetooth gateway:

```text
                 Battery
                    │
              Power Management
               ┌────┴────┐
               ▼         ▼
          ESP32-S3      WROOM
            Minji      BT Gateway
               │         │
               └── GND ──┘
```

Prinsip target:

- Satu battery system.
- Common ground antara ESP32-S3 dan WROOM.
- Hindari memberi WROOM dari rail 3.3 V Minji tanpa verifikasi kapasitas regulator.
- Jika expansion board memiliki rail 5 V stabil dengan arus cukup, WROOM mungkin dapat mengambil supply dari rail tersebut.
- Jika tidak, gunakan regulator/boost terpisah yang sesuai.
- Proteksi backfeed perlu ditentukan setelah topologi power aktual diketahui; jangan menganggap satu dioda otomatis menyelesaikan semua skenario.

---

# 6. Bluetooth Audio Expansion

## ESP32-WROOM-32D Bluetooth Gateway

Status: 🧪 **EXPERIMENTAL**

WROOM dipakai sebagai subsystem Bluetooth terpisah karena Bluetooth audio tidak dijalankan oleh jalur utama Minji ESP32-S3.

Target arsitektur:

```text
Minji ESP32-S3
      │
      │ I2S audio
      ▼
ESP32-WROOM-32D
      │
      │ Bluetooth A2DP
      ▼
Bluetooth Speaker
```

Current direction untuk jalur audio Minji → WROOM:

- target sample rate: **24 kHz**
- sample width: **32-bit**
- channel: **mono LEFT**

Konfigurasi receiver lama 44.1 kHz tidak dianggap konfigurasi final Minji.

### Planned extension: Bluetooth microphone / Remote Ear

Status: 🗺️ **PLANNED**

A2DP hanya menangani output media. Microphone pada Bluetooth speaker/headset membutuhkan profile/jalur lain seperti HFP/HSP jika ingin dipakai sebagai input Minji.

Konsep ini belum dianggap fitur siap pakai.

---

# 7. Minji Software / Service Architecture

Minji tidak didesain sebagai chatbot ESP32 generik. Target arsitekturnya memisahkan percakapan utama dari layanan lokal khusus.

```text
                    ┌─────────────────────┐
                    │ Conversation backend│
                    │      VN / cloud     │
                    └─────────┬───────────┘
                              │
                              ▼
                         MINJI ESP32-S3
                              │
                    special/local requests
                              │
                              ▼
                    ┌─────────────────────┐
                    │    Genius Local     │
                    │ radio / media/tools │
                    └─────────────────────┘
```

Design rule:

- Percakapan normal tetap ditangani conversation backend.
- Request khusus layanan Indonesia/local dapat diarahkan ke Genius/local tools.
- Multimedia lokal tidak perlu bergantung pada SD card jika server lokal menyediakan source media.
- MCP/cloud tooling dapat menjadi extension layer tanpa membuat ESP32 harus menangani seluruh logika layanan sendiri.

---

# 8. Feature Reality Check

## Confirmed Working

- ✅ ESP32-S3 N16R8 boot dan operasi normal.
- ✅ LCD Minji 128×160.
- ✅ Minji Face Engine / eye-based UI.
- ✅ Blink dan gaze/pupil movement.
- ✅ Internal microphone.
- ✅ Internal speaker / voice output.
- ✅ Wi-Fi.
- ✅ VN conversation backend yang digunakan Minji.
- ✅ Genius local service integration.
- ✅ Local radio / media playback melalui Genius.
- ✅ Battery-only operation.
- ✅ Charging melalui expansion board.
- ✅ Device diagnostics / Black Box reset history.
- ✅ Volume control GPIO implementation pada Minji.
- ✅ True stop local audio/media.

## Experimental

- 🧪 ESP32-WROOM-32D Bluetooth A2DP gateway.
- 🧪 I2S audio handoff Minji → WROOM.
- 🧪 Final dual-ESP power distribution.

## Under Investigation

- 🔎 Expansion-board 5 V rail saat battery-only.
- 🔎 Maximum safe external load untuk WROOM.
- 🔎 Battery sense GPIO / ADC source.
- 🔎 Final anti-backfeed arrangement untuk service/debug USB.

## Planned

- 🗺️ Bluetooth microphone as external/remote ear.
- 🗺️ Selectable internal vs Bluetooth audio destination.
- 🗺️ Translator role/tool.
- 🗺️ Calculator/tool-assisted arithmetic.
- 🗺️ Additional local/MCP services.

---

# 9. Minji Diagnostics / Black Box

Minji mempunyai diagnostic/Black Box yang dipakai untuk membantu membedakan reset dan masalah power.

Contoh kategori yang sudah terlihat:

- `POWERON`
- `USB`
- `WATCHDOG`
- `PANIC`
- `BROWNOUT`

Rolling history membantu membedakan restart normal dari reset karena watchdog/panic/brownout.

Catatan UI: jika dashboard menampilkan label **Server Uptime** untuk uptime perangkat, label itu sebaiknya dibaca/diperbaiki menjadi **Minji Uptime / Device Uptime** agar tidak rancu dengan uptime proses Genius Server.

---

# 10. Development Environment

Environment Minji yang digunakan pada PC pengembangan:

- Windows
- workspace utama: `D:\xiaozhi-esp32`
- ESP-IDF environment utama: **v5.5.5**
- terminal kerja: **LAB Terminal**

Workflow terminal dibuat agar pekerjaan ESP-IDF tidak perlu berpindah-pindah shell/environment secara manual.

> Upstream XiaoZhi dapat bergerak ke versi ESP-IDF yang berbeda. Dokumentasi Minji harus mengikuti environment yang benar-benar dipakai untuk build Minji, bukan otomatis mengikuti versi upstream terbaru.

---

# 11. Troubleshooting Quick Reference

## LCD putih setelah flash

Periksa:

1. board variant yang dipilih,
2. driver LCD,
3. pin mapping,
4. orientation/display init,
5. backlight GPIO.

Jika device masih bersuara atau tersambung Wi-Fi, jangan langsung menyimpulkan LCD rusak.

## Device restart berulang

Periksa Black Box:

- `POWERON` → power cycle/start biasa,
- `USB` → native USB boot/reset,
- `WATCHDOG` → task/firmware stall,
- `PANIC` → software exception/crash,
- `BROWNOUT` → supply drop perlu diperiksa.

## Minji tidak mendengar saat media aktif

Pastikan jalur media tidak memblokir microphone/listening pipeline. Minji telah mengalami kasus media playback yang membuat perangkat tampak "budek"; perbaikannya harus mempertahankan kemampuan wake/listen ketika media berjalan.

## WROOM tidak menerima audio

Sebelum menyalahkan Bluetooth:

- verifikasi breadboard row/continuity,
- common ground,
- BCLK,
- WS/LRCK,
- DATA,
- sample format/rate,
- dan pastikan receiver tidak masih memakai konfigurasi 44.1 kHz lama.

---

# 12. Documentation Gaps — What We Still Need

Bagian ini sengaja dibiarkan sebagai checklist supaya terlihat jelas apa yang masih kurang dari "Kitab Minji".

- [ ] Foto keseluruhan **Minji Expansion Board V1.7 — sisi atas** dengan anotasi.
- [ ] Foto keseluruhan **Minji Expansion Board V1.7 — sisi bawah** dengan anotasi.
- [ ] Foto **ESP32-S3 N16R8 terpasang pada expansion board**.
- [ ] Foto lokasi **USB-C kiri / Native USB**.
- [ ] Foto lokasi **USB-C kanan / CH340C UART**.
- [ ] Foto konektor battery + polaritas yang sudah diverifikasi.
- [ ] Foto charging LED dan lokasi power section.
- [ ] Foto LCD/display connector.
- [ ] Foto microphone/speaker connector/path.
- [ ] Foto wiring **Minji ↔ WROOM** setelah jalur I2S final.
- [ ] Diagram pinout expansion board yang sudah diverifikasi.
- [ ] Hasil ukur battery-only rail: voltage dan titik ukur.
- [ ] Hasil uji current/load rail untuk WROOM.
- [ ] Final dual-ESP power schematic.
- [ ] Final A2DP wiring dan audio format setelah berhasil stabil.
- [ ] Screenshot dashboard System Health/Black Box yang sudah memakai label `Minji Uptime`.
- [ ] `CHANGELOG.md` versi Minji berdasarkan release/tag yang benar-benar digunakan.

---

# 13. Upstream & Credits

Minji dibangun di atas proyek open-source **XiaoZhi ESP32**.

Upstream:

- https://github.com/78/xiaozhi-esp32

Lisensi repository tetap mengikuti file [LICENSE](LICENSE) yang ada pada project ini.

Tujuan dokumentasi Minji bukan menggantikan seluruh dokumentasi XiaoZhi, tetapi mencatat secara presisi **hardware, konfigurasi, perilaku, dan fitur yang benar-benar digunakan oleh Minji**.

---

## README AS REALME

Jika README dan hardware berbeda, yang harus diperbaiki adalah dokumentasinya — bukan kenyataannya.

**CONFIRMED means tested. EXPERIMENTAL means experimental. PLANNED means not available yet.**
