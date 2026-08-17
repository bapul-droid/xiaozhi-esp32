# MINJI_PROJECT_STATE

**Snapshot:** 17 Agustus 2026  
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

## 9. WROOM / A2DP — Working Baseline

Companion:

`ESP32-WROOM-32D / ESP32-D0WD-V3 -> Bluetooth Classic A2DP -> Edifier M260`

Project Windows:

`D:\esp32-a2dp-test`

Bridge I2S PCM -> A2DP sudah pernah CONFIRMED WORKING dan stabil.

Format working lama:

- 24 kHz
- 32-bit
- mono-left
- WROOM mengonversi ke 44.1 kHz stereo untuk A2DP.

Bluetooth startup WROOM ditunda sekitar 8 detik agar tidak mengganggu asosiasi Wi-Fi Minji.

UART tambahan yang sudah terpasang:

```text
Minji G18 <- WROOM GPIO17/TX2
Minji G3  -> WROOM GPIO16/RX2
```

Penambahan UART tidak menyebabkan crash/reset, tetapi fungsi kontrol dua arah belum menjadi fitur final.

## 10. Audio Separation — Target Aktif

Target final:

```text
TTS / conversation
        -> speaker internal Minji

Radio / music Genius
        -> WROOM
        -> A2DP
        -> Edifier M260
```

### Source aktif

CMake mengonfirmasi file yang benar-benar digunakan:

```text
main/audio/audio_service.cc
main/genius_client/audio_stream_client.cc
```

File berikut bukan implementation aktif yang harus dijadikan dasar:

```text
main/genius_client/audio_service.cc
main/audio/audio_stream_client.cc
```

Jangan kembali mengedit file duplikat yang tidak masuk build.

### genius_media

Genius stream sudah menandai radio/music dengan `genius_media = true`, dan flag tersebut dibawa melalui decode queue sampai output task.

Dengan demikian firmware sudah dapat membedakan audio normal dengan radio/music Genius.

Root cause double internal audio ditemukan di active `main/audio/audio_service.cc`: output stage sebelumnya selalu menjalankan `codec_->OutputData(task->pcm)` tanpa memanfaatkan klasifikasi `task->genius_media`.

## 11. Audio Separation V1 — Half Success

V1 telah dibuat, build berhasil, flash berhasil, dan diuji pada hardware.

Hasil:

```text
TTS:
Minji   = suara
Edifier = suara

Radio:
Minji   = DIAM
Edifier = DIAM
```

CONFIRMED dari eksperimen ini:

- Genius media classification bekerja.
- Radio HTTP stream bekerja (`HTTP 200`, `audio/ogg`).
- Opus queue/decode berjalan.
- Internal radio mute/separation bekerja.

Yang belum berhasil:

`media-only PCM -> working WROOM I2S path`

Jadi V1 menyelesaikan separuh arsitektur separation: radio sudah tidak bocor ke speaker internal, tetapi dedicated output ke WROOM belum benar.

## 12. Koreksi Wiring Audio — Sangat Penting

Checkpoint awal pernah mencatat mapping working lama:

```text
Minji G15 -> WROOM GPIO27 BCLK
Minji G16 -> WROOM GPIO14 WS/LRCK
Minji G7  -> WROOM GPIO22 DATA
Minji GND -> WROOM GND
```

Audio Separation V1 kemudian mencoba dedicated output:

```text
GPIO17 = BCLK
GPIO13 = WS
GPIO14 = DATA
```

Mapping `17/13/14` **tidak boleh dianggap wiring final**. Konfigurasi terkait jalur ini pernah berhubungan dengan bootloop dan dibuat berdasarkan asumsi, bukan mapping hardware working terakhir.

Checkpoint terbaru juga mengoreksi asumsi rewiring: pada eksperimen A2DP terakhir yang benar-benar menghasilkan audio Edifier, perubahan dilakukan **di sisi WROOM**, bukan dengan memindahkan sisi Minji seperti asumsi berikutnya.

Wiring working tersebut masih secara fisik terpasang; yang sempat dilepas hanya USB power WROOM.

**KEPUTUSAN: wiring audio sekarang FROZEN sampai mapping A2DP working lama direkonstruksi dari meja/chat eksperimen lama.**

Jangan rewire berdasarkan `17/13/14`.

## 13. Audio Separation V2 — Belum Boleh Langsung Dibuat

Sebelum patch/flash berikutnya, rekonstruksi terlebih dahulu kondisi A2DP working terakhir:

1. Pin output Minji yang benar-benar digunakan saat WORKING.
2. Pin WROOM sebelum dipindah.
3. Pin WROOM setelah dipindah.
4. Mapping BCLK / WS / DATA yang menghasilkan suara Edifier.
5. Firmware/commit WROOM yang digunakan saat kondisi WORKING.

Setelah mapping tersebut confirmed, Audio Separation V2 menggunakan prinsip:

```text
task->genius_media == false
    -> codec internal Minji

task->genius_media == true
    -> dedicated WORKING WROOM I2S path
    -> jangan codec internal
```

Jangan membuat controller I2S ketiga. Eksperimen V1 menunjukkan kedua controller ESP32-S3 sudah digunakan speaker TX dan microphone RX.

## 14. Repository / Baseline Penting

### Firmware Minji

Repository: `bapul-droid/xiaozhi-esp32`

Checkpoint sebelumnya mencatat branch kerja `minji-main`; repository GitHub saat snapshot dokumentasi memiliki default branch `main`.

Commit penting:

- `c2f5dd5` — `feat: add Minji diagnostics, telemetry, watchdog recovery, and media wake`
- `09c5744` — `fix: prevent VAD from immediately stopping media`

Tag tested:

- `minji-media-wake-v3-tested-20260815`

File utama:

- `main/application.cc`
- `main/genius_client/genius_client.cc`
- `main/genius_client/genius_client.h`
- `main/boards/bread-compact-wifi-lcd/compact_wifi_board_lcd.cc`
- `main/audio/audio_service.cc`
- `main/genius_client/audio_stream_client.cc`

### Genius Server

Repository: `bapul-droid/minji-genius-server`  
Branch: `main`

Commit penting:

- `770e2a1` — `chore: keep server environment private`
- `76913c0` — `fix: synchronize dashboard media state with active stream`
- `3057b88` — Quick Action Stop Media

### WROOM A2DP Bridge

Repository: `bapul-droid/minji-a2dp-bridge`  
Branch: `master`

Commit penting:

- `b416d80` — `working: stable Minji I2S PCM to Edifier A2DP`
- `f9b58d3` — `chore: stop tracking generated build artifacts`
- `394c6c0` — `feat: delay Bluetooth startup until Minji Wi-Fi is ready`

## 15. Closed / Jangan Dibuka Ulang Tanpa Alasan

- Battery percentage presisi — CLOSED.
- Expansion switch — CLOSED, posisi KIRI.
- Basic battery ADC investigation — CLOSED FOR NOW.
- GPIO11 relation to battery telemetry — CONFIRMED.
- Media play -> immediate stop bug — FIXED.
- Media Wake V3 — STABLE.
- Radio HTTP/Opus path — CONFIRMED.
- `genius_media` classification — CONFIRMED.
- Internal radio mute — CONFIRMED.
- WROOM -> Edifier A2DP capability — CONFIRMED.
- Third I2S controller approach — ABANDONED.
- `17/13/14` assumed final wiring — REJECTED.
- Minji Math v0.1 — ABANDONED.
- XiaoZhi OTA — OFF.
- Black Box — KEEP.

## 16. Active Work Queue

### Priority 1 — Audio Separation V2

Langkah pertama **bukan coding, patch, flash, atau rewiring**.

Rekonstruksi mapping A2DP working terakhir dari meja eksperimen lama. Setelah mapping hardware confirmed, baru desain media-only routing melalui working WROOM path tanpa membuat I2S controller tambahan.

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

Jangan langsung patch bila belum ada hasil observasi.

### Parked — Natural Barge-in / AEC

Tetap menjadi target jangka panjang, tetapi tidak mengganggu baseline stabil sampai jalur AEC dan dukungan server dipahami.

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
10. Audio wiring sekarang FROZEN sampai mapping A2DP lama ditemukan.
11. `genius_media` menjadi dasar Audio Separation.
12. Jangan membuat I2S controller ketiga.
13. Role/personality/memory diubah melalui console terlebih dahulu bila memungkinkan.
14. Jangan mengejar battery %, FULL indicator, atau presisi yang tidak dibutuhkan.
15. Jika Minji restart/crash, periksa Black Box terlebih dahulu sebelum menebak penyebab.

## 18. Posisi Berhenti

Minji bukan lagi dalam fase membuat perangkat dasar bekerja. Core device, server, media, wake, telemetry, Black Box, battery monitoring, dan A2DP sudah mempunyai baseline yang nyata.

Target pengembangan utama sekarang adalah **Audio Separation V2**:

- TTS/conversation tetap melalui speaker internal Minji.
- Radio/music Genius hanya melalui WROOM -> A2DP -> Edifier.

**NEXT ACTION yang benar:** kembali ke checkpoint/meja A2DP lama dan kunci mapping working terakhir sebelum patch, flash, atau mengubah wiring apa pun.
