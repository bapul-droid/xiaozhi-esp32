# MINJI_PROJECT_STATE

**Snapshot:** 17 Agustus 2026 — Audio Separation + BT Control CLOSED  
**Repository:** `bapul-droid/xiaozhi-esp32`

Dokumen ini adalah state terpadu proyek Minji dari checkpoint berbagai meja. Fakta terbaru mengoreksi checkpoint lama bila terjadi konflik.

## 1. Status Umum

Minji saat ini memiliki baseline yang stabil dan usable. Core device, Genius Server, media, wake, telemetry, Black Box, battery monitoring, dan WROOM/A2DP sudah mempunyai baseline nyata.

Komponen utama:

- Minji: ESP32-S3, board `bread-compact-wifi-lcd`, flash 16 MB, PSRAM 8 MB, LCD 128x160, speaker/mic internal, expansion battery/charging board.
- Genius Server: Debian, `192.168.1.89:8000`, service `genius.service`.
- Companion audio: ESP32-WROOM-32D / ESP32-D0WD-V3 -> Bluetooth Classic A2DP -> Edifier M260.

Firmware stabil tidak diubah tanpa tujuan eksperimen yang terukur. OTA XiaoZhi tetap dimatikan agar firmware custom Minji tidak tertimpa.

## 2. Media Wake V3 — Stable Baseline

CONFIRMED:

- Radio iRadio dapat dimainkan.
- Online music/YouTube dapat dimainkan.
- Media dimulai setelah TTS benar-benar selesai.
- Listening session ditutup sebelum radio/music lokal dimulai.
- Wake word tetap aktif ketika media berjalan.
- Wake word `Minji` menghentikan stream aktif dan membuka sesi percakapan baru.
- LCD dapat bangun saat wake word terdeteksi.
- Dashboard Genius mengikuti status `PLAYING` / `STOPPED` secara realtime.
- Quick Action **Stop Media** berfungsi dari dashboard/HP.

Bug lama media `play -> stop` telah diperbaiki. Root cause-nya adalah VAD membaca suara TTS/speaker sebagai suara pengguna.

Tag perangkat teruji:

`minji-media-wake-v3-tested-20260815`

## 3. Barge-in — Belum Full Natural Barge-in

Yang tersedia sekarang adalah **wake-word interruption**, bukan natural full-duplex barge-in.

CONFIRMED:

`Minji -> stop radio/music -> listen`

Belum dikonfirmasi bahwa user dapat berbicara biasa tanpa wake word ketika TTS sedang berlangsung dan langsung menginterupsi Minji.

Natural barge-in masih membutuhkan investigasi:

- microphone aktif selama TTS;
- realtime listening;
- AEC;
- VAD yang membedakan suara user dari speaker;
- server-side abort.

Referensi SeekAudio tidak dianggap patch siap pakai. Media Wake V3 tidak boleh dirusak untuk mengejar barge-in sebelum jalur AEC dipahami.

## 4. Genius Server / Web Console

Server:

- Debian
- `192.168.1.89:8000`
- `genius.service`
- `uvicorn app:app --host 0.0.0.0 --port 8000`

Sudah berjalan:

- device registration;
- heartbeat;
- volume telemetry;
- battery telemetry;
- crash report;
- hardware diagnostics;
- media monitor realtime;
- calculation session;
- conversation history;
- Quick Action Stop Media.

Web Console telah berkembang sampai v0.3 Black Box.

Role/personality/memory/bahasa diubah melalui console terlebih dahulu bila tidak membutuhkan perubahan firmware.

## 5. Black Box / Crash Diagnostics

Black Box adalah komponen permanen dan wajib dipertahankan.

Retention: **rolling 24 jam**.

Counter:

- Total Reset
- Brownout
- Watchdog
- Panic
- Power On

Reset History menyimpan konteks seperti waktu, reset reason, last event, last state, uptime, free SRAM, dan minimum SRAM.

Crash report firmware -> Debian sudah terbukti bekerja.

Pengamatan terakhir:

- POWERON: 9
- BROWNOUT: 0
- WATCHDOG: 0
- PANIC: 0

Black Box digunakan sebagai flight recorder pasif. Jangan sengaja membuat Minji crash hanya untuk mengisi dashboard.

## 6. Battery / Expansion Board — Closed for Now

Battery asli Minji: **Li-ion 18650 3100 mAh**.

Charging sampai termination telah diuji. USB power meter mencapai `0.00 A` setelah penuh, sehingga charger/cut-off expansion board bekerja.

### Expansion switch

Posisi operasi normal yang dipilih: **KIRI**.

Contoh pembacaan switch kiri:

- Web voltage: 4.200 V
- GPIO11: sekitar 3117 mV / raw 4069
- GPIO12: sekitar 11 mV / raw 14
- Charging: YES

Switch kanan menghasilkan battery telemetry tidak valid dan tidak digunakan untuk operasi normal.

### GPIO11 / GPIO12

GPIO11 CONFIRMED berkaitan dengan jalur battery ADC/telemetry. Ini menggantikan checkpoint awal yang masih menyebut GPIO11 hanya kandidat.

GPIO12 berubah drastis mengikuti kondisi/switch tetapi belum dianggap indikator charging/full yang presisi.

Battery percentage dan FULL detection presisi tidak dikejar. Dashboard cukup memakai voltage praktis.

Patch web membatasi tampilan voltage atas ke **4.200 V** tanpa mengubah raw ADC/telemetry.

File patch terkait:

- `PATCH_MINJI_BATTERY_VOLTAGE_WEB_V1.py`
- `Minji_Battery_Voltage_Web_V1.zip`

## 7. GPIO Explorer

GPIO Explorer v2 berhasil build, flash, dan berjalan sebagai ADC Observer.

Karakteristik:

- sampling 500 ms;
- averaging 16 sampel;
- snapshot 10 detik;
- threshold CHANGE: 35 mV atau 45 raw;
- observer-first;
- tidak meng-drive kandidat sebagai output;
- tidak memasang pull-up/pull-down.

ADC mapping runtime yang ditemukan:

```text
GPIO1  -> ADC1 CH0
GPIO2  -> ADC1 CH1
GPIO3  -> ADC1 CH2
GPIO8  -> ADC1 CH7
GPIO9  -> ADC1 CH8
GPIO10 -> ADC1 CH9
GPIO11 -> ADC2 CH0
GPIO12 -> ADC2 CH1
GPIO13 -> ADC2 CH2
GPIO14 -> ADC2 CH3
GPIO17 -> ADC2 CH6
```

Pelajaran penting: perubahan ADC besar tidak otomatis berarti pin mempunyai fungsi. Floating pin dapat menghasilkan perubahan ekstrem.

Battery investigation sekarang diparkir. GPIO Explorer v2.1 tidak diperlukan kecuali muncul kebutuhan diagnostik hardware baru.

## 8. Minji Math

### v0.1 — Abandoned

Ditinggalkan sebagai desain final karena Minji terlalu cepat merespons setiap angka, misalnya `Oke, saya catat. Lanjut.`, sehingga dapat berbicara bersamaan dengan user dan mengganggu input berikutnya.

### v0.2 STRICT

Sudah dibuat dan dipasang di Debian dengan desain:

1. Masuk Calculation Mode.
2. User menyebut angka.
3. Angka dicatat diam-diam.
4. Jeda user bukan tanda selesai.
5. Koreksi angka diperbolehkan.
6. Perhitungan baru dilakukan setelah cue eksplisit seperti `oke hitung` / `sekarang hitung`.

Reminder ketika user terlalu lama diam belum dimasukkan.

Status: **belum mendapatkan pengujian suara final yang memadai**.

Pekerjaan ini tetap server-side, terutama `server/genius/minji/calculation.py`, dan sebisa mungkin tidak menyentuh firmware stabil Minji.

## 9. WROOM / A2DP / Audio Separation — FINAL WORKING BASELINE

Companion:

`ESP32-WROOM-32D / ESP32-D0WD-V3 -> Bluetooth Classic A2DP -> Edifier M260`

Project Windows:

`D:\\esp32-a2dp-test`

Status akhir: **CONFIRMED WORKING, STABLE, CLOSED**.

Arsitektur final:

```text
TTS / conversation / news
        -> speaker internal Minji

Radio / online music Genius
        -> jika Edifier CONNECTED:
           I2S media route -> WROOM -> A2DP -> Edifier

        -> jika Edifier DISCONNECTED/OFF:
           otomatis kembali ke speaker internal Minji
```

Format audio menuju WROOM:

- 24 kHz;
- 32-bit I2S;
- mono-left;
- WROOM meneruskan ke Edifier melalui A2DP.

Tidak dibuat controller I2S ketiga. Firmware merutekan ulang channel TX speaker yang sudah ada.

## 10. Wiring Final — CONFIRMED dan FROZEN

### Audio I2S

```text
Minji GPIO17 -> WROOM GPIO27  BCLK
Minji GPIO13 -> WROOM GPIO14  WS/LRCK
Minji GPIO14 -> WROOM GPIO22  DATA
Minji GND    -> WROOM GND
```

### UART control

```text
Minji GPIO18 TX -> WROOM GPIO16 RX
Minji GPIO3  RX <- WROOM GPIO17 TX
Minji GND       -> WROOM GND
```

Mapping ini telah diuji pada hardware nyata. Radio/music terdengar di Edifier, TTS tetap internal, UART dua arah bekerja, dan tidak terjadi crash.

**KEPUTUSAN:** wiring final dikunci. Jangan kembali ke mapping lama G15/G16/G7 menuju WROOM. G15/G16/G7 sekarang tetap menjadi jalur speaker internal Minji.

## 11. Bluetooth Companion Control — CONFIRMED

Protokol UART:

- baud 115200;
- `PING`;
- `BT STATUS`;
- `BT CONNECT`;
- `BT DISCONNECT`;
- `BT VOLUME 0..100`.

MCP Minji:

- `self.bluetooth.get_status`;
- `self.bluetooth.connect`;
- `self.bluetooth.disconnect`;
- `self.bluetooth.set_volume`.

Pengujian suara yang berhasil:

- “Putuskan speaker Bluetooth” -> `AUTO=0`, A2DP/AVRCP disconnected.
- “Hubungkan kembali speaker Bluetooth” -> `AUTO=1`, A2DP lalu AVRCP connected.
- “Kecilkan volume ke 50%” -> `OK BT VOLUME 50`.
- Permintaan status -> mengenali `EDIFIER M260` dan volume aktual.

Fallback yang berhasil:

```text
BT EVENT DISCONNECTED
-> I2S TX kembali G15/G16/G7
-> media lanjut melalui speaker internal

BT EVENT CONNECTED
-> I2S TX pindah G17/G13/G14
-> media lanjut melalui Edifier
```

Disconnect melalui perintah suara menonaktifkan auto-reconnect sampai perintah connect diberikan. Power-off Edifier dengan `AUTO=1` tetap memungkinkan reconnect otomatis saat Edifier hidup kembali.

Discovery/pairing perangkat Bluetooth baru belum menjadi fitur. Perintah “scan perangkat sekitar” saat ini hanya menghasilkan status perangkat paired aktif.

## 12. Audio Separation — Riwayat Kegagalan dan Keputusan

### V1 — half success, ditinggalkan

- TTS masih keluar di dua speaker.
- Radio internal berhasil mute.
- Radio belum sampai WROOM.

### V2 awal — route berhasil, leakage internal

- Radio mencapai WROOM/Edifier.
- Pin output internal lama masih terikat GPIO matrix sehingga radio juga bocor ke speaker internal.

### V2.1 — final

`NoAudioCodec::SetOutputGpio()` diperbaiki agar:

- menyimpan route lama;
- rollback bila reconfiguration gagal;
- melepas pin route lama dengan `gpio_reset_pin()`;
- `RestoreOutputGpio()` mengembalikan G15/G16/G7.

Hasil final:

- radio/music hanya Edifier ketika connected;
- TTS/news/conversation hanya internal;
- Edifier offline tidak lagi membuat Minji diam;
- tidak ada warning I2S controller occupied;
- tidak ada crash pada pengujian final.

Build fixes yang ditemukan:

- WROOM status UART harus membatasi nama perangkat menjadi `%.64s` agar lolos `-Werror=format-truncation`;
- Minji `wroom_companion.cc` memerlukan `#include <driver/gpio.h>`.

## 13. Online Music Server Unicode Fix

Online music sempat gagal meskipun search berhasil. Endpoint:

`GET /api/radio-stream/music`

menghasilkan HTTP 500 dan stream 0 byte.

Root cause:

```text
UnicodeEncodeError: latin-1 cannot encode combining character U+0301
```

Judul YouTube seperti `eńau feat. Ari Lesmana...` dimasukkan ke header:

```python
"X-Radio-Station": display_name
```

Header tersebut dihapus dari `server/genius/api/radio_proxy.py`. Setelah restart `genius.service`, online music berhasil mengalir ke Edifier.

Commit server:

- `6a3e88c` — `fix: prevent unicode crash in media stream header`
- remote branch: `agent/server-thermal-health`

## 14. Repository / Baseline Penting

### Firmware Minji

Repository: `bapul-droid/xiaozhi-esp32`  
Branch: `main`

Commit final:

- `0b2f21e` — `feat: add Bluetooth companion control and audio fallback`

Commit baseline terkait:

- `4176269` — diagnostics, telemetry, watchdog recovery, dan media wake;
- `b401ae4` — mencegah VAD langsung menghentikan media.

File utama:

- `main/application.cc`
- `main/audio/audio_service.cc`
- `main/audio/audio_codec.cc/.h`
- `main/audio/codecs/no_audio_codec.cc/.h`
- `main/genius_client/genius_client.cc`
- `main/genius_client/wroom_companion.cc/.h`
- `main/mcp_server.cc`

### WROOM A2DP Bridge

Repository: `bapul-droid/minji-a2dp-bridge`  
Branch: `master`

Commit final:

- `6d4f809` — `feat: add UART Bluetooth companion control`

Baseline sebelumnya:

- `b416d80` — stable Minji I2S PCM to Edifier A2DP;
- `394c6c0` — delay Bluetooth startup sampai Wi-Fi Minji siap.

### Genius Server

Repository: `bapul-droid/minji-genius-server`

Commit terkait sesi ini:

- `6a3e88c` — mencegah Unicode crash pada media stream header, branch `agent/server-thermal-health`.

## 15. Closed / Jangan Dibuka Ulang Tanpa Alasan

- Battery percentage presisi — CLOSED.
- Expansion switch — CLOSED, posisi KIRI.
- Basic battery ADC investigation — CLOSED FOR NOW.
- Media play -> immediate stop bug — FIXED.
- Media Wake V3 — STABLE.
- Radio HTTP/Opus path — CONFIRMED.
- Online music Unicode header crash — FIXED.
- `genius_media` classification — CONFIRMED.
- Audio Separation V2.1 — FINAL/CONFIRMED.
- Wiring audio G17/G13/G14 -> WROOM 27/14/22 — FINAL/FROZEN.
- UART G18/G3 -> WROOM 16/17 — FINAL/FROZEN.
- WROOM status/connect/disconnect/volume — CONFIRMED.
- Automatic internal fallback — CONFIRMED.
- Third I2S controller approach — ABANDONED.
- Minji Math v0.1 — ABANDONED.
- XiaoZhi OTA — OFF.
- Black Box — KEEP.

## 16. Active Work Queue

### Priority 1 — Stability Observation

Gunakan konfigurasi BT final dalam pemakaian normal. Jika terjadi reset/crash, periksa Black Box terlebih dahulu.

Catatan observasi: minimum free SRAM ketika media + kontrol BT pernah turun sekitar 5–7 KB. Belum terjadi crash pada pengujian final, tetapi angka ini perlu dipantau sebelum menambahkan fitur berat baru.

Status UART saat ini dipoll sekitar setiap 2 detik dan menghasilkan log yang cukup ramai. Ini dapat dibersihkan kemudian, tetapi bukan blocker fungsi.

### Priority 2 — Minji Math v0.2 STRICT

Lakukan uji suara nyata. Target: angka dicatat diam-diam dan Minji baru bicara setelah cue hitung, data tidak jelas, atau user meminta daftar catatan.

### Priority 3 — Wake/TTS Interruption

Uji observasi khusus:

```text
Minji sedang TTS panjang
-> panggil "Minji"
-> cari log Wake word detected
-> cari log Abort speaking
```

### Parked — Natural Barge-in / AEC

Tetap menjadi target jangka panjang, tetapi tidak mengganggu baseline stabil sampai jalur AEC dan dukungan server dipahami.

### Parked — Bluetooth Discovery/Pairing Baru

Saat ini WROOM mengendalikan Edifier paired yang sudah dikenal. Scan, memilih, dan pairing speaker Bluetooth baru belum diimplementasikan.

## 17. Aturan Kerja Project State

1. Jangan mengubah firmware stabil tanpa tujuan terukur.
2. Black Box tetap hidup.
3. Jangan menggunakan dua port USB Minji secara bersamaan.
4. Expansion switch tetap KIRI.
5. Perubahan Genius Server disebarkan melalui GitHub dan `git pull`.
6. Eksperimen berisiko menggunakan branch/PR.
7. Jangan percaya file duplikat sebelum memeriksa CMake/build path.
8. Jangan mengubah wiring berdasarkan asumsi source code.
9. Working hardware state lebih kuat daripada mapping teoretis.
10. Wiring final audio dan UART FROZEN sesuai mapping CONFIRMED pada Bagian 10.
11. `genius_media` menjadi dasar Audio Separation.
12. Jangan membuat I2S controller ketiga.
13. Role/personality/memory diubah melalui console terlebih dahulu bila memungkinkan.
14. Jangan mengejar battery %, FULL indicator, atau presisi yang tidak dibutuhkan.
15. Jika Minji restart/crash, periksa Black Box terlebih dahulu sebelum menebak penyebab.

## 18. Posisi Berhenti

Integrasi Bluetooth Minji–WROOM–Edifier telah selesai dan ditutup sebagai working baseline.

Kondisi akhir:

- Minji mengenali WROOM dan Edifier melalui UART;
- connect/disconnect/status/volume dapat dikontrol melalui suara;
- radio/music pindah ke Edifier ketika connected;
- media otomatis kembali ke speaker internal ketika Edifier disconnected/off;
- reconnect mengembalikan media ke Edifier;
- TTS/news/conversation tetap internal;
- kedua repository firmware sudah bersih dan tersinkron dengan remote;
- wiring final sudah dikunci.

**NEXT ACTION:** tidak ada patch BT tambahan. Gunakan konfigurasi ini dalam pemakaian normal dan pantau Black Box/minimum SRAM. Fokus proyek berikutnya kembali ke antrean non-BT, terutama Minji Math v0.2 atau observasi wake/TTS interruption.
