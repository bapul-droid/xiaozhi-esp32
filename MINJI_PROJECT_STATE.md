# MINJI_PROJECT_STATE

**Snapshot:** 17 Agustus 2026 — Bluetooth Companion FINAL CLOSED  
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
- `BT VOLUME 0..100`;
- `BT SCAN`;
- output `BT DEVICE ...` dan `BT SCAN END`.

MCP Minji:

- `self.bluetooth.get_status`;
- `self.bluetooth.connect`;
- `self.bluetooth.disconnect`;
- `self.bluetooth.set_volume`;
- `self.bluetooth.scan`.

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

### Auto-reconnect dan scan — FINAL/CONFIRMED

Bug lifecycle lama membuat WROOM hanya dapat tersambung kembali setelah restart ketika Edifier dimatikan lalu dinyalakan lagi. Root cause-nya adalah retry langsung ke alamat lama serta perubahan state lokal saat stack Bluetooth masih berada pada state berbeda.

Perbaikan final:

- kehilangan Edifier mengembalikan WROOM ke discovery berbasis heartbeat;
- discovery tidak diulang secara rekursif dari callback GAP;
- timeout koneksi meminta stack menutup sesi sebelum retry;
- perintah connect memaksa discovery target baru;
- perintah disconnect membatalkan discovery dan menetapkan `AUTO=0`;
- hasil scan manual dideduplikasi berdasarkan MAC, maksimal 8 perangkat;
- nama, RSSI, class-of-device, dan MAC diteruskan ke Minji melalui UART.

Pengujian hardware nyata:

- Edifier OFF -> `BT EVENT DISCONNECTED`;
- WROOM -> `Reconnect discovery started: heartbeat retry`;
- Edifier ON -> target `fc:e8:06:dd:d9:f2` ditemukan;
- A2DP dan AVRCP kembali connected tanpa restart WROOM;
- scan suara menemukan dan disebutkan Minji: `EDIFIER M260`, HP `baPuL_F3`, dan `Mi Box`;
- setelah scan, perintah suara berhasil menyambungkan Edifier kembali;
- tidak terjadi crash/reset.

RSSI `-129 dBm` berarti paket hasil discovery tidak membawa RSSI valid; bukan nilai kekuatan sinyal nyata.

**Batas fitur:** scan/listing perangkat sekitar sudah selesai. Memilih dan pairing target audio baru selain Edifier tetap tidak diimplementasikan karena bukan kebutuhan milestone ini.

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
- `9e025f4` / `1114845` — koleksi dan penyajian hasil scan Bluetooth
- `d72edf6` — expose `self.bluetooth.scan` ke Minji

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
- `cef4a7c` — `fix: recover A2DP reconnect and report Bluetooth scans`

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
- Auto-reconnect Edifier tanpa restart WROOM — FIXED/CONFIRMED.
- Bluetooth scan dan penyebutan nama/RSSI/MAC oleh Minji — FINAL/CONFIRMED.
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

### Parked — Pairing Target Audio Baru

Scan/listing perangkat sekitar sudah FINAL. Memilih dan pairing speaker Bluetooth baru selain target Edifier tetap diparkir karena tidak dibutuhkan untuk menutup milestone BT.

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
- auto-reconnect menemukan dan menyambungkan kembali Edifier tanpa restart WROOM;
- Minji dapat memindai dan menyebut perangkat Bluetooth sekitar beserta RSSI/MAC;
- TTS/news/conversation tetap internal;
- kedua repository firmware sudah bersih dan tersinkron dengan remote;
- wiring final sudah dikunci.

**NEXT ACTION:** tidak ada patch BT tambahan. Gunakan konfigurasi ini dalam pemakaian normal dan pantau Black Box/minimum SRAM. Fokus proyek berikutnya kembali ke antrean non-BT, terutama Minji Math v0.2 atau observasi wake/TTS interruption.

## Smart Home BARDI / Tuya — FINAL / CONFIRMED (2026-08-18)

### CONFIRMED
- BARDI Wall Switch EU 3 Gang berhasil diintegrasikan ke ekosistem Minji.
- Perangkat menggunakan akun Smart Life / Tuya Cloud.
- Tuya Cloud Project yang cocok untuk akun Smart Life ini berada di Western America Data Center.
- Smart Life account berhasil dilink ke Tuya Developer Project.
- Tiga channel perangkat terdeteksi dan dapat dikontrol independen:
  - switch_1 = Teras
  - switch_2 = Ruang Tamu
  - switch_3 = Ruang TV
- Tuya Developer Web -> physical BARDI switch: CONFIRMED.
- Genius Debian -> Tuya API READ status: CONFIRMED.
- Genius Debian -> Tuya API WRITE ON/OFF: CONFIRMED.
- Genius Web Console -> BARDI status + ON/OFF: CONFIRMED.
- XiaoZhi voice -> MCP tool -> Genius -> Tuya -> BARDI: FINAL / CONFIRMED.
- Voice test confirmed:
  "Nyalakan ruang TV"
  -> self.smarthome.set_light
  -> GeniusClient SetBardiSwitch(room=ruang_tv, gang=3, state=ON)
  -> POST /api/home/bardi/switch
  -> Tuya success=true
  -> physical light ON
  -> Minji response: "Lampu ruang TV sudah dinyalakan."

### Architecture
User voice
-> XiaoZhi / LLM
-> self.smarthome.set_light
-> GeniusClient
-> Genius Debian /api/home/bardi/switch
-> Tuya Cloud
-> BARDI Wall Switch
-> physical light

### Security / Design Decision
- Tuya Access ID / Access Secret are NOT stored in Minji firmware.
- Tuya credentials stay on Genius Debian only.
- Firmware only knows the Genius local API.
- This keeps cloud credentials out of the ESP32 and allows the backend smart-home provider to be changed later without redesigning Minji firmware.

### Genius Console
SMART HOME — BARDI panel added with:
- Teras ON/OFF
- Ruang Tamu ON/OFF
- Ruang TV ON/OFF
- live status refresh

### Files
Firmware:
- main/genius_client/genius_client.h
- main/genius_client/genius_client.cc
- main/mcp_server.cc

Genius Server:
- server/genius/integrations/bardi_tuya.py
- server/genius/api/home_bardi.py
- server/genius/main.py
- server/genius/api/console.py

### Notes
- Official Smart Life control depends on Tuya Cloud / internet.
- Local-LAN BARDI control was not required for this milestone.
- Smart-home integration does not require adopting a full Home Assistant-style platform.
- Genius exposes a small purpose-built smart-home adapter instead.

## Battery & Power Telemetry — FINAL / CONFIRMED (2026-08-18)

### FINAL / CONFIRMED
- GPIO11 = battery voltage sensing.
- GPIO12 = charging-state sensing, active LOW.
- Battery voltage calibration finalized using physical multimeter measurements.
- Genius Server uses piecewise calibration from real Minji hardware measurements.
- Battery percentage estimation is enabled using the calibrated voltage.
- Genius Console now displays:
  - Battery Level (%)
  - Battery Voltage
  - Charging YES / NO
  - ADC GPIO11 raw/mV
  - CHRG GPIO12 raw/mV
  - Telemetry age

### Physical Validation
Charging:
- Battery Level: 47%
- Voltage: 3.782 V
- GPIO11: 2825 mV (raw 3469)
- GPIO12: 11 mV (raw 15)
- Charging: YES / CHARGING

Charger disconnected:
- Battery Level: 33%
- Voltage: 3.684 V
- GPIO11: 2750 mV (raw 3351)
- GPIO12: 1598 mV (raw 1886)
- Charging: NO

Independent multimeter comparison:
- Multimeter: approximately 3.74 V
- Genius telemetry observed: approximately 3.713 V
- Difference approximately 27 mV (~0.7%)

### Locked Hardware Interpretation
- GPIO11 = BAT_ADC
- GPIO12 = CHRG
- GPIO12 LOW (~10–20 mV observed) = CHARGING
- GPIO12 HIGH (~1.6 V observed) = NOT CHARGING

### Notes
- Battery percentage is an estimate derived from battery voltage and is expected to move with charging/load voltage behavior.
- Battery voltage is calibrated specifically against measurements from this Minji unit.
- Charging detection has been physically confirmed in both charger-connected and charger-disconnected states.

## Battery Self-Awareness via MCP — FINAL / CONFIRMED (2026-08-18)

### FINAL / CONFIRMED
Minji sekarang dapat membaca dan menjelaskan status baterainya sendiri melalui voice interaction.

MCP tool:
- `self.battery.get_status`

Data source:
- Genius Server `/api/debug/battery`
- GPIO11 = battery voltage sensing
- GPIO12 = charging-state sensing (active LOW)

Flow confirmed:

User voice
-> XiaoZhi
-> `self.battery.get_status`
-> GeniusClient::GetBatteryStatus()
-> Genius Server battery telemetry
-> MCP result
-> spoken response by Minji

### Physical / Voice Validation

Test command:
- "Berapa baterai kamu sekarang?"

Observed result:
- MCP tool selected correctly: `self.battery.get_status`
- Genius telemetry read successfully.
- Battery: 90%
- Voltage: approximately 4.10 V
- Charging: YES
- Minji responded verbally:
  "Baterai saya 90 persen, sekitar 4.10 volt, dan sedang diisi daya."

### Decision
Battery information must always come from live Genius telemetry.
Minji/XiaoZhi must not guess battery percentage, voltage, or charging state.

### Status
- Battery voltage sensing: FINAL
- Battery percentage estimation: FINAL
- Charging detection: FINAL
- Genius battery API: FINAL
- MCP battery status tool: FINAL
- Natural-language battery query: FINAL
- Spoken battery response: FINAL

Battery telemetry and Minji battery self-awareness milestone CLOSED.

## Power Management V1 — FINAL / CONFIRMED (2026-08-19)

### FINAL / CONFIRMED
Minji sekarang memiliki battery warning otomatis berbasis telemetry Genius Server.

Threshold:
- Battery <= 20% dan NOT CHARGING -> LOW warning sekali.
- Battery <= 10% dan NOT CHARGING -> CRITICAL warning sekali.
- CRITICAL memiliki prioritas lebih tinggi daripada LOW.

Charging behavior:
- GPIO12 active LOW tetap menjadi sumber status charging.
- Begitu CHARGING terdeteksi, LOW dan CRITICAL warning latch langsung di-reset.
- Reset tidak menunggu battery percentage naik.
- Selama charging aktif, LOW/CRITICAL warning tidak dikirim.
- Jika charger dicabut ketika battery masih berada di bawah threshold, kondisi dievaluasi kembali dan warning boleh aktif kembali.

Anti-spam:
- Warning pada level yang sama hanya dikirim sekali per cycle.
- Repeated battery telemetry pada kondisi yang sama tidak menyebabkan warning berulang.

Notification path:
Genius battery telemetry
-> PowerManager
-> Genius command_queue
-> action `notify`
-> GeniusClient::HandleCommand()
-> Application::Alert()
-> LCD status / emotion Minji

Levels:
- battery_low -> status `BATERAI RENDAH`, emotion `sad`
- battery_critical -> status `BATERAI KRITIS`, emotion `cancel`
- battery_full path sudah disiapkan di firmware, tetapi full/termination notification otomatis belum diaktifkan.

### Validation
Simulated LOW:
- 19%
- not charging
- `[POWER] ... LOW 19% warning queued`
- Minji displayed BATERAI RENDAH.

Simulated CRITICAL:
- 9%
- not charging
- `[POWER] ... CRITICAL 9% warning queued`
- Minji displayed BATERAI KRITIS.

Charging reset:
- real GPIO12 returned to approximately 10–12 mV
- charging=True
- `[POWER] ... charging detected, battery warning latches reset`

Real telemetry resumed after simulation and replaced test data correctly.

### Current Power Management Scope
Included:
- LOW warning
- CRITICAL warning
- charging suppression
- immediate charging reset
- anti-spam latch

Not included:
- automatic shutdown
- software-controlled charger cutoff
- automatic full-battery notification
- aggressive display/power reduction based on battery level

### Superseded Battery Notes
Older PROJECT_STATE notes stating that battery percentage was not pursued or that GPIO12 was not yet a precise charging indicator are superseded by the FINAL battery telemetry validation from 2026-08-18 and Power Management V1 validation from 2026-08-19.

### Commits
Firmware:
- `d2aebf2` — `feat: add local battery warning notifications`

Genius Server:
- `367cf95` — `feat: add Minji battery power management warnings`

Power Management V1 milestone CLOSED.

---

## CHECKPOINT 2026-08-22 ? LOCAL OTA / ALARM / VOICE

### CONFIRMED

- Local OTA melalui Genius Server berhasil penuh.
- Firmware Minji memakai OTA endpoint lokal:
  `http://192.168.1.89:8000/xiaozhi/ota/`
- Genius OTA response kompatibel dengan alur XiaoZhi dan berhasil menawarkan firmware lokal.
- Firmware 2.4.5 berhasil didownload, ditulis ke OTA partition, reboot, lalu tervalidasi.
- 2.4.5 confirmed dengan log:
  - `Current is the latest version`
  - `Running partition: ota_1`
  - `Marking firmware as valid`
- OTA firmware kini dapat dilakukan tanpa USB.
- Firmware 2.4.5 membawa wake/listening guard:
  `Wake word received while already listening; keeping current session`
- Remote Device Log console pada Genius Debian berhasil direstore.
- Alarm backend berhasil direstore:
  - `/api/alarms`
  - `AlarmRegistry`
  - SQLite pada Genius Server
- Data alarm lama tetap tersimpan di `genius.db`.
- Alarm Web Console kembali tampil.
- Alarm lama `Tes alarm Minji` terbaca sebagai `MISSED` pada UI.
- Humanity Battery Meter tetap dipertahankan.
- Genius Server tetap menjadi source of truth untuk state/history Minji; ESP32 tidak digunakan sebagai penyimpanan permanen untuk alarm/history.

### UNDER INVESTIGATION

- Setelah firmware 2.4.5 masuk melalui OTA, Minji tetap dapat:
  - mendeteksi wake word,
  - membuka WebSocket,
  - mendapatkan Session ID,
  - masuk state `listening`.
- Namun percakapan setelah wake masih dapat diam/tidak menghasilkan respons.
- Wake/listening guard 2.4.5 sudah aktif, tetapi belum menyelesaikan akar masalah voice.
- Belum dipastikan apakah perbedaan metode OTA vs full USB flash ikut memengaruhi kondisi voice.

### NEXT TEST ? OTA VS USB

- Gunakan binary 2.4.5 YANG SAMA, jangan rebuild sebelum A/B test.
- SHA256 binary 2.4.5:
  `F851111B3E28649428EBF5018ED73C6478285F7DDF599399EA22489779DE8132`
- Flash binary tersebut secara langsung melalui USB.
- Setelah USB flash, tes fisik:
  `wake "Minji" -> bicara -> STT -> TTS`.
- Bandingkan perilaku binary identik:
  - 2.4.5 via OTA
  - 2.4.5 via USB
- Jika USB normal tetapi OTA bisu, investigasi state OTA / partition / persistent config.
- Jika keduanya tetap bisu, fokus kembali ke voice/session pipeline.

### ALARM NEXT

Lifecycle target:

`ACTIVE -> RINGING/TRIGGERED -> COMPLETED`

Alternatif akhir:

- `MISSED`
- `CANCELLED`

Rencana kontrol:
- Double-click tombol menjadi kandidat STOP alarm lokal.
- Stop alarm harus bekerja lokal terlebih dahulu, lalu ACK ke Genius Server.
- Genius Server tetap menyimpan jadwal, status, dan riwayat alarm.

### GENIUS SERVER RECOVERY 2026-08-22

Commit recovery Debian:

- `e88ed83` ? feat: add remote device log receiver
- `787a9c1` ? feat: add remote device log console
- `13d0d73` ? feat: restore Minji console and alarm registry

Catatan:
- Genius Server saat recovery berada pada detached HEAD.
- Voice server fix masih lokal/uncommitted:
  - `server/genius/minji/session.py`
  - `server/genius/minji/stt.py`
- Backup voice fix:
  `~/minji-safe/`

### USB BASELINE 2.4.7 — CONFIRMED HEALTHY

Test date:
- 2026-08-22

Firmware:
- Version: `2.4.7`
- USB flash test: PASS
- SHA256 `build/xiaozhi.bin`:
  `2AA8AAAACFBFBA7C461443EC56C79FF1A6FB021C16B3302CD184638F7D381A22`

Voice pipeline CONFIRMED:
- Wake word `Minji` detected.
- State transition `connecting -> listening` normal.
- User speech successfully received.
- State transition `listening -> speaking` normal.
- TTS/audio output normal.
- Conversation successfully continued into the next listening turn.

Physical conversation evidence:
- `Application: >> Minji`
- `Application: << Iya, ada yang bisa saya bantu?`
- `Application: >> selamat menikmati`
- `Application: << Terima kasih!`

Therefore:
- Minji is NOT mute on the recovered 2.4.7 USB baseline.
- Wake -> Listening -> STT -> Server -> TTS -> Listening is CONFIRMED operational.

### XIAOZHI / TENCLASS UPSTREAM — CONFIRMED

Boot log on healthy 2.4.7:

- `Compiled CONFIG_OTA_URL: https://api.tenclass.net/xiaozhi/ota/`
- `OTA URL actually used: https://api.tenclass.net/xiaozhi/ota/`
- `MINJI OTA URL: https://api.tenclass.net/xiaozhi/ota/`

Decision:
- Keep XiaoZhi/Tenclass OTA/upstream path intact on the known-good baseline.
- Genius Server remains Minji's companion/control/state server.
- Do NOT replace or remove the XiaoZhi/Tenclass upstream path again without an isolated A/B test.

### REMOTE DEVICE LOG — CONFIRMED RESTORED

Firmware Remote Device Log sender is operational.

Observed:
- `POST http://192.168.1.89:8000/api/device-log`
- HTTP `200`
- Server accepted batches of 8 log entries.
- Web Console `RECENT DEVICE LOG — MINJI` populated normally.
- Observed Web Console buffer: `160/5000`.

Recovered/optimized configuration:
- Remote log queue: 32 entries.
- Batch size: 8.
- Tag buffer: 24 bytes.
- Message buffer: 128 bytes.

### GENIUS TELEMETRY — CONFIRMED

Web Console after 2.4.7 USB boot showed:
- Device ONLINE.
- Wi-Fi RSSI telemetry operational.
- Battery telemetry operational.
- Alarm registry/UI preserved.
- Remote Device Log operational.

### BASELINE DECISION

Firmware `2.4.7` is the new KNOWN-GOOD recovery baseline.

Freeze this baseline before further alarm/media/OTA experiments.

Do not reintroduce the firmware-side alarm experiment or the 2.4.5 wake/listening guard until tested independently from this baseline.
