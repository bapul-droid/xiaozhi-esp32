Minji Firmware Diagnostics v1

ISI:
- volume heartbeat membaca volume AudioCodec aktual (bukan hardcoded 70)
- boot/reset report ke POST /api/console/crash/report
- reset reason: POWERON/BROWNOUT/PANIC/WDT/dll
- uptime saat report
- last state marker (RTC memory)
- last event marker (RTC memory)
- free SRAM
- minimum SRAM
- marker wake_word_detected untuk kasus crash setelah Minji dibangunkan

INSTALL:
Salin folder main/ di paket ini ke root D:\xiaozhi-esp32\
dan izinkan overwrite file dengan path yang sama.

Lalu dari LAB Terminal:
  cd D:\xiaozhi-esp32
  idf.py build

Jika build sukses:
  idf.py -p COM10 flash monitor

CATATAN:
Crash Recorder v1 adalah telemetry reset, bukan ESP core dump penuh.
RTC marker membantu software reset/panic/watchdog; pada power loss/brownout berat,
isi RTC marker tidak dijamin selalu bertahan. Reset reason dari ESP-IDF tetap dikirim.
