# MINJI REFERENCE BACKLOG

Dokumen ini menyimpan sumber referensi yang berpotensi berguna untuk pengembangan Minji di masa depan agar tidak hilang di antara sesi eksperimen.

Referensi di sini **bukan NEXT ACTION** dan **bukan pekerjaan aktif** sampai diputuskan untuk dikerjakan.

## Mic V2 / Far-field / Wake During Playback

### XiaoZhi ESP32-S3 dual-microphone board

Sumber utama:

https://www.reddit.com/r/Esphome/comments/1pvedl6/xiaozhi_esp32s3_voice_assistant_board_new/

Alasan disimpan:

- Menunjukkan varian perangkat XiaoZhi ESP32-S3 yang menggunakan dua microphone.
- Relevan sebagai pembanding hardware/firmware untuk rencana Mic V2 Minji.
- Layak dibedah untuk melihat bagaimana input multichannel ditangani pada board XiaoZhi yang memang dirancang dual-mic.
- ES7210/multichannel audio ADC disebut sebagai kandidat penting dari penelusuran awal; detail implementasinya harus diverifikasi dari source sebelum dijadikan dasar perubahan Minji.

Konteks masalah Minji yang ingin dibandingkan di masa depan:

- wake-word sensitivity kadang sangat baik tetapi belum konsisten;
- Minji lebih sulit dipanggil ketika playback/media sedang aktif, bahkan ketika output dipindahkan ke Bluetooth speaker dengan volume kecil;
- hardware mic internal tidak dianggap rusak dan tetap menjadi baseline;
- arah yang pernah dibahas adalah **menambah** microphone, bukan mengganti mic internal;
- target akhirnya bukan sekadar mic lebih sensitif, tetapi kemampuan membedakan suara user, playback Minji, dan noise lingkungan.

Area riset terkait:

- far-field voice assistant;
- wake word detection during music playback;
- acoustic echo cancellation (AEC);
- noise suppression;
- double-talk detection;
- microphone array / dual-mic processing;
- beamforming;
- barge-in during audio playback.

### Keputusan saat referensi disimpan

- **REFERENCE / FUTURE INVESTIGATION ONLY.**
- Tidak mengubah firmware Minji sekarang.
- Tidak mengubah gain mic sekarang.
- Tidak mengaktifkan AEC hanya berdasarkan referensi ini.
- Tidak mengganti mic internal.
- Layout/kabel Minji dan WROOM akan dirapikan terlebih dahulu agar koneksi mekanis dan kemungkinan interferensi/crosstalk tidak mengacaukan baseline.
- Firmware Minji dianggap sudah mencapai baseline final; fitur/perbaikan baru sebisa mungkin dikerjakan server-side kecuali memang membutuhkan perubahan device.

Saat investigasi mic dilanjutkan, mulai dari referensi ini dan literatur smart-speaker/far-field, bukan hanya issue microphone XiaoZhi standar.
