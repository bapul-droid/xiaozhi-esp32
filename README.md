# MINJI

> **README AS REALME** — yang tertulis di sini harus mencerminkan perilaku Minji yang benar-benar terlihat di hardware, bukan sekadar kemampuan yang secara teori mungkin didukung upstream.

Minji adalah custom build dari [XiaoZhi ESP32](https://github.com/78/xiaozhi-esp32) yang dikembangkan menjadi perangkat AI fisik dengan wajah sendiri, hardware tetap, layanan lokal Genius, diagnostik/Black Box, operasi baterai, multimedia lokal, dan companion Bluetooth.

## Minji Ecosystem

Minji sekarang terdiri dari beberapa repository yang merupakan **satu project yang saling terhubung**:

| Repository | Peran | Status |
|---|---|---|
| **[Minji Core Firmware](https://github.com/bapul-droid/xiaozhi-esp32)** | ESP32-S3 utama, Face Engine, display, audio, device integration | ✅ ACTIVE |
| **[Minji Genius Server](https://github.com/bapul-droid/minji-genius-server)** | local services, multimedia, dashboard, Black Box | ✅ ACTIVE |
| **[Minji A2DP Bridge](https://github.com/bapul-droid/minji-a2dp-bridge)** | ESP32-WROOM-32D Bluetooth audio companion | 🧪 EXPERIMENTAL |

```text
                  Conversation Backend
                         │
                         ▼
                ┌─────────────────┐
                │      MINJI      │
                │    ESP32-S3     │
                │   Core Device   │
                └───────┬─────────┘
                        │
             ┌──────────┴──────────┐
             │                     │
       Local Network            I2S Audio
             │                     │
             ▼                     ▼
   ┌──────────────────┐   ┌──────────────────┐
   │  Genius Server   │   │ ESP32-WROOM-32D │
   │ Media / Black Box│   │   A2DP Bridge    │
   └──────────────────┘   └────────┬─────────┘
                                   │ Bluetooth
                                   ▼
                              BT Speaker
```

---

## Status Legend

| Status | Arti |
|---|---|
| ✅ **CONFIRMED** | Sudah diuji langsung pada hardware Minji dan bekerja. |
| 📋 **MANUFACTURER / SELLER SPEC** | Informasi listing/produk; belum otomatis dianggap terbukti. |
| 🔎 **UNDER INVESTIGATION** | Sedang diukur/ditelusuri. Jangan dijadikan dasar wiring final. |
| 🧪 **EXPERIMENTAL** | Implementasi ada/sedang diuji, tetapi belum final. |
| 🗺️ **PLANNED** | Roadmap; belum tersedia sebagai fitur siap pakai. |
| 📷 **PHOTO NEEDED** | Dokumentasi sudah diketahui kebutuhannya, foto final belum tersedia/terpilih. |

**Aturan dokumentasi:** `supported` bukan sinonim `working`. Jika ditulis **CONFIRMED**, Minji benar-benar pernah menjalankannya.

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
| Audio | I2S microphone + internal speaker |
| Connectivity | Wi-Fi + companion Bluetooth terpisah |

Status: ✅ **CONFIRMED**

## Minji Expansion Board

Nama project:

**ESP32-S3 Smart Expansion Board V1.7**

Nama lain yang ditemukan di marketplace:

> **ESP32-S3 Smart Expansion Board V1.7 | Integrated I2S Audio Amplifier with TFT Display Development Base Board N16R8 Applicable**

Board ini merupakan bagian penting dari Minji: display/audio, koneksi USB/UART, battery/charging, dan power integration berada di sekitar platform ini.

### Confirmed on Minji

- ✅ ESP32-S3 N16R8 berjalan pada expansion board.
- ✅ LCD 1.8 inch 128×160 bekerja.
- ✅ Microphone dan internal speaker bekerja.
- ✅ Minji dapat hidup battery-only melalui expansion board.
- ✅ Charging battery telah terlihat bekerja; LED charging menyala.
- ✅ Dua jalur USB menunjukkan perilaku berbeda pada Windows dan Black Box.

### Manufacturer / Seller Specification

- 📋 Nama produk: Smart Expansion Board V1.7 untuk ESP32-S3 N16R8.
- 📋 Listing menyebut I2S audio amplifier dan TFT display development base board.

### Under Investigation

- 🔎 Rail power yang tersedia saat battery-only.
- 🔎 Apakah rail 5 V stabil tersedia untuk beban eksternal.
- 🔎 Kapasitas arus untuk ESP32-WROOM companion.
- 🔎 Topologi charging/power-path lengkap.
- 🔎 Titik supply WROOM yang aman dari backfeed saat USB/debug terhubung.

📷 **PHOTO NEEDED:** expansion board top/bottom view dengan anotasi konektor dan power section.

---

# 2. Known Minji Hardware Behavior

## USB-C / Serial Behavior

### Port kiri — Native USB ESP32-S3

Status: ✅ **CONFIRMED**

- Windows: **COM8** pada unit pengujian.
- Black Box mencatat boot/reset jalur ini sebagai **USB**.
- Digunakan sebagai native USB ESP32-S3.

### Port kanan — CH340C / UART

Status: ✅ **CONFIRMED**

- Windows: **COM10** pada unit pengujian.
- Menggunakan CH340C/UART bridge.
- Black Box melihat jalur ini sebagai **POWERON**, bukan native USB.

### Battery / Powerbank

Status: ✅ **CONFIRMED**

- Minji tetap hidup tanpa USB menggunakan battery melalui expansion board.
- Powerbank/battery terlihat sebagai **POWERON** di Black Box.

### Safety rule during testing

> Hindari dua sumber USB sekaligus selama eksperimen power sampai topologi final benar-benar diketahui.

📷 **PHOTO NEEDED:** foto kedua port USB-C dengan label Native USB dan CH340C/UART.

---

# 3. Display — Known Working Configuration

Status: ✅ **CONFIRMED**

Minji menggunakan LCD **1.8 inch 128×160**.

## Known failure: LCD putih

Gejala yang pernah terjadi:

- ESP32-S3 boot.
- Audio/Wi-Fi bekerja.
- Backlight menyala.
- LCD tetap putih.

Pada Minji, kondisi tersebut pernah disebabkan ketidakcocokan konfigurasi display/board; bukan otomatis panel rusak.

## Known Minji LCD pin mapping

| Signal | GPIO |
|---|---:|
| LCD SDA | 47 |
| LCD RES | 45 |
| LCD DC | 40 |
| LCD CS | 41 |
| LCD BLK | 42 |

Baseline board configuration:

```text
CONFIG_BOARD_TYPE_BREAD_COMPACT_WIFI_LCD=y
```

---

# 4. Minji Face Engine

Status: ✅ **CONFIRMED**

Face Engine adalah salah satu pembeda utama Minji dari UI XiaoZhi generik. Wajah Minji menggunakan konsep **eye-only face** tanpa mulut dan dirancang agar ekspresi dapat berkembang tanpa menjadikan core UI satu blok besar yang sulit dirawat.

### Confirmed behavior

- ✅ Eye-based Minji face aktif pada LCD.
- ✅ Blink animation.
- ✅ Gaze / pupil movement.
- ✅ Pergerakan dibuat ringan untuk ESP32-S3.
- ✅ Face Engine menjadi dasar pengembangan ekspresi Minji.

### Design direction

Face Engine disiapkan agar tampilan/ekspresi baru dapat berkembang sebagai bagian terpisah dari UI utama. Target jangka panjangnya adalah perubahan ekspresi, warna, dan behavior wajah tanpa harus mengacak keseluruhan firmware display.

```text
Minji UI / Device State
          │
          ▼
     Face Engine
          │
    ┌─────┴─────┐
    │           │
  Eyes       Emotion
    │           │
    └─────┬─────┘
          ▼
      LCD 128×160
```

### Visual documentation

Arsip Google Drive sudah memiliki **original design reference / concept baseline** yang menjadi referensi awal wajah Minji. Gambar tersebut belum diperlakukan sebagai foto hardware aktual.

📷 **PHOTO NEEDED:** foto LCD Minji aktual dengan Face Engine aktif untuk dipasangkan dengan concept/reference image.

---

# 5. Audio Hardware

Status: ✅ **CONFIRMED** untuk audio internal.

## I2S microphone

| Signal | GPIO |
|---|---:|
| MIC WS | 4 |
| MIC SCK | 5 |
| MIC DIN | 6 |

## Internal speaker

| Signal | GPIO |
|---|---:|
| SPK DOUT | 7 |
| SPK BCLK | 15 |
| SPK LRCK | 16 |

Minji dapat berbicara melalui speaker internal dan menerima suara melalui microphone internal.

### Known audio behavior

- Audio noise pernah menjadi issue investigasi.
- Media playback dan voice/listening pipeline harus tetap dipisahkan dengan benar.
- Minji pernah mengalami kondisi media aktif membuat perangkat tampak "budek"; ini merupakan behavior penting untuk regression testing.

---

# 6. Battery & Power Architecture

## Current confirmed state

Status: ✅ **CONFIRMED**

```text
Battery
   │
   ▼
Expansion Board V1.7
   │
   ▼
ESP32-S3 Minji
```

Minji sudah dapat beroperasi battery-only.

## Target dual-ESP architecture

Status: 🔎 **UNDER INVESTIGATION**

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
- Jangan memberi WROOM dari rail 3.3 V tanpa verifikasi kapasitas regulator.
- Rail 5 V expansion baru boleh dianggap solusi setelah voltage/current diuji.
- Proteksi backfeed ditentukan setelah topologi aktual diketahui.

📷 **PHOTO NEEDED:** battery connector/polarity, charging section, dan final dual-ESP power wiring setelah tervalidasi.

---

# 7. Bluetooth Audio Expansion

## ESP32-WROOM-32D Bluetooth Gateway

Status: 🧪 **EXPERIMENTAL**

WROOM adalah companion processor untuk membawa audio Minji ke Bluetooth speaker melalui Classic Bluetooth A2DP.

```text
Minji ESP32-S3
      │
      │ I2S
      ▼
ESP32-WROOM-32D
      │
      │ Bluetooth A2DP
      ▼
Bluetooth Speaker
```

Current investigation untuk input Minji → WROOM:

- **24 kHz**
- **32-bit**
- **mono LEFT**

Konfigurasi generic/receiver lama **44.1 kHz** tidak dianggap konfigurasi final Minji.

Standalone WROOM telah membuktikan sisi Bluetooth/A2DP dapat bekerja, tetapi jalur fisik Minji → WROOM belum dipromosikan menjadi `CONFIRMED` sampai audio Minji berhasil end-to-end.

### Technical reference

➡️ **[Minji A2DP Bridge](https://github.com/bapul-droid/minji-a2dp-bridge)** — firmware WROOM, status pengujian, audio format, wiring yang sedang divalidasi, dan dokumentasi Bluetooth companion.

### Planned: Bluetooth microphone / Remote Ear

Status: 🗺️ **PLANNED**

A2DP menangani output. Input microphone Bluetooth membutuhkan profile/jalur lain seperti HFP/HSP dan belum dianggap fitur siap pakai.

📷 **PHOTO NEEDED:** final Minji ↔ WROOM wiring setelah BCLK/WS/DATA dan power benar-benar lolos pengujian.

---

# 8. Genius Local Services

Status: ✅ **CONFIRMED** untuk integrasi yang sudah digunakan Minji.

Genius adalah **local service layer** Minji. Ia menangani fungsi yang lebih tepat berada di server lokal daripada ditanam seluruhnya ke firmware ESP32.

```text
Normal conversation
        │
        └──> Conversation backend

Local / multimedia / diagnostic
        │
        └──> Genius Local Server
                ├── radio / media
                ├── dashboard
                ├── Black Box
                └── local tools
```

Confirmed pada project Minji:

- ✅ Genius local service integration.
- ✅ Local radio/media path.
- ✅ Dashboard/device information.
- ✅ Black Box diagnostic integration.

MCP/local-tool expansion tetap dicatat sebagai evolving/planned sampai jalur end-to-end yang dipakai Minji benar-benar tervalidasi.

### Server reference

➡️ **[Minji Genius Server](https://github.com/bapul-droid/minji-genius-server)** — implementation/reference untuk local services, multimedia, dashboard, Black Box, persistent-memory component, dan perkembangan MCP/local tools.

---

# 9. Minji Diagnostics / Black Box

Status: ✅ **CONFIRMED**

Black Box digunakan untuk melihat behavior perangkat tanpa bergantung hanya pada USB serial log.

Kategori reset yang sudah terlihat:

- `POWERON`
- `USB`
- `WATCHDOG`
- `PANIC`
- `BROWNOUT`

Rolling history membantu membedakan power cycle biasa dari watchdog, panic, atau brownout.

Black Box juga berguna ketika menguji battery-only, charging, perbedaan dua USB-C, dan nantinya shared-power Minji + WROOM.

> Jika dashboard menampilkan **Server Uptime** untuk uptime perangkat, label sebaiknya dibedakan menjadi **Minji Uptime / Device Uptime** agar tidak rancu dengan uptime Genius Server.

### Black Box implementation/reference

➡️ **[Minji Genius Server — Black Box / local diagnostics](https://github.com/bapul-droid/minji-genius-server)**

📷 **PHOTO NEEDED:** screenshot dashboard Black Box/System Health final dengan label uptime yang tidak rancu.

---

# 10. Feature Reality Check

## Confirmed Working

- ✅ ESP32-S3 N16R8 boot dan operasi normal.
- ✅ LCD 128×160.
- ✅ Minji Face Engine / eye-based UI.
- ✅ Blink + gaze/pupil movement.
- ✅ Internal microphone.
- ✅ Internal speaker / voice output.
- ✅ Wi-Fi.
- ✅ VN conversation backend yang digunakan Minji.
- ✅ Genius local service integration.
- ✅ Local radio/media playback melalui Genius.
- ✅ Battery-only operation.
- ✅ Charging melalui expansion board.
- ✅ Black Box/reset history.
- ✅ Volume-control GPIO implementation.
- ✅ True stop local audio/media.

## Experimental

- 🧪 ESP32-WROOM-32D Bluetooth A2DP gateway.
- 🧪 I2S handoff Minji → WROOM.
- 🧪 Final dual-ESP power distribution.

## Under Investigation

- 🔎 Expansion-board 5 V rail saat battery-only.
- 🔎 Maximum safe external load untuk WROOM.
- 🔎 Battery sense GPIO / ADC source.
- 🔎 Final anti-backfeed arrangement.

## Planned

- 🗺️ Bluetooth microphone / Remote Ear.
- 🗺️ Selectable internal vs Bluetooth audio destination.
- 🗺️ Translator role/tool.
- 🗺️ Calculator/tool-assisted arithmetic.
- 🗺️ Additional MCP/local services.

---

# 11. Development Environment

Current Minji PC development baseline:

- Windows
- workspace: `D:\xiaozhi-esp32`
- ESP-IDF: **v5.5.5**
- primary shell: **LAB Terminal**

Upstream XiaoZhi dapat bergerak ke versi ESP-IDF lain. Dokumentasi Minji mengikuti environment yang benar-benar digunakan untuk build Minji.

---

# 12. Troubleshooting Quick Reference

## LCD putih setelah flash

Periksa board variant, driver LCD, pin mapping, orientation/display init, dan backlight GPIO. Jika audio/Wi-Fi masih bekerja, jangan langsung menyimpulkan LCD rusak.

## Device restart berulang

Gunakan Black Box untuk membedakan `POWERON`, `USB`, `WATCHDOG`, `PANIC`, dan `BROWNOUT`.

➡️ Detail server/dashboard: **[Minji Genius Server](https://github.com/bapul-droid/minji-genius-server)**

## Minji tidak mendengar saat media aktif

Pastikan media playback tidak memblokir microphone/listening pipeline. Kondisi ini pernah terjadi pada Minji dan harus diperlakukan sebagai regression case.

## WROOM tidak menerima audio

Sebelum menyalahkan Bluetooth, verifikasi:

- breadboard row/continuity,
- common ground,
- BCLK,
- WS/LRCK,
- DATA,
- sample format/rate,
- receiver tidak masih memakai asumsi 44.1 kHz lama.

➡️ Detail subsystem: **[Minji A2DP Bridge](https://github.com/bapul-droid/minji-a2dp-bridge)**

---

# 13. Documentation Gaps / PHOTO NEEDED

Checklist ini sengaja dipertahankan agar yang belum terdokumentasi terlihat jelas dan tidak diganti dengan tebakan.

- [ ] 📷 Expansion Board V1.7 — top view beranotasi.
- [ ] 📷 Expansion Board V1.7 — bottom view beranotasi.
- [ ] 📷 ESP32-S3 N16R8 terpasang pada expansion board.
- [ ] 📷 USB-C kiri / Native USB.
- [ ] 📷 USB-C kanan / CH340C UART.
- [ ] 📷 Battery connector + polaritas terverifikasi.
- [ ] 📷 Charging LED + power section.
- [ ] 📷 LCD/display connector.
- [ ] 📷 Microphone/speaker path.
- [ ] 📷 Face Engine actual LCD photo.
- [ ] 📷 Minji ↔ WROOM final wiring.
- [ ] 📷 Genius dashboard / Black Box final screenshot.
- [ ] Diagram pinout expansion board terverifikasi.
- [ ] Battery-only rail voltage + titik ukur.
- [ ] Current/load test rail untuk WROOM.
- [ ] Final dual-ESP power schematic.
- [ ] Final A2DP wiring + audio format setelah stabil.
- [ ] `CHANGELOG.md` Minji berdasarkan release/tag yang benar-benar digunakan.

### Existing visual archive

Google Drive sudah memiliki sejumlah foto/screenshot dan original Face Engine design reference. Arsip tersebut sedang dipilah; hanya gambar yang identitas dan konteksnya jelas yang layak dipindahkan menjadi dokumentasi resmi.

**Jangan memasukkan foto eksperimen sebagai recommended wiring.** Foto kegagalan boleh dipakai untuk troubleshooting jika diberi konteks yang jelas.

---

# 14. Repository Cross-Reference

## Minji Core Firmware

**This repository** — ESP32-S3 device, Face Engine, hardware integration, audio, power behavior, dan device-side integration.

## Minji Genius Server

https://github.com/bapul-droid/minji-genius-server

Local services, multimedia, dashboard, Black Box, persistent-memory component, dan local/MCP tooling.

## Minji A2DP Bridge

https://github.com/bapul-droid/minji-a2dp-bridge

ESP32-WROOM-32D Bluetooth audio companion dan jalur eksperimen Minji → I2S → A2DP → Bluetooth speaker.

---

# 15. Upstream & Credits

Minji dibangun di atas proyek open-source **XiaoZhi ESP32**:

https://github.com/78/xiaozhi-esp32

Lisensi repository tetap mengikuti [LICENSE](LICENSE).

Tujuan dokumentasi Minji bukan menggantikan seluruh dokumentasi XiaoZhi, tetapi mencatat secara presisi **hardware, konfigurasi, perilaku, dan fitur yang benar-benar digunakan Minji**.

---

## README AS REALME

Jika README dan hardware berbeda, yang harus diperbaiki adalah dokumentasinya — bukan kenyataannya.

**CONFIRMED means tested. EXPERIMENTAL means experimental. PLANNED means not available yet.**
