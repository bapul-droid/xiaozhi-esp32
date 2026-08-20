#ifndef REMOTE_DEVICE_LOG_H
#define REMOTE_DEVICE_LOG_H

#include <cstddef>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class RemoteDeviceLog {
public:
    static RemoteDeviceLog& GetInstance();

    // Pasang hook ESP-IDF logger. Aman dipanggil lebih dari sekali.
    void Start();

    // Dipakai GeniusClient agar proses POST log tidak merekam dirinya sendiri.
    void SetSuppressed(bool suppressed);

    // Ambil maksimal max_entries dari antrean dan bentuk payload server:
    // {"device_id":"...","logs":[{"uptime_ms":...,"level":"I","tag":"...","message":"..."}]}
    // Return false jika antrean kosong.
    bool BuildBatchJson(
        const std::string& device_id,
        std::string& json_body,
        size_t max_entries = 40
    );

private:
    RemoteDeviceLog() = default;
    RemoteDeviceLog(const RemoteDeviceLog&) = delete;
    RemoteDeviceLog& operator=(const RemoteDeviceLog&) = delete;

    struct Entry {
        uint32_t uptime_ms;
        char level[2];
        char tag[32];
        char message[192];
    };

    static int LogVprintf(const char* format, va_list args);
    void EnqueueFormatted(const char* line);

    QueueHandle_t queue_ = nullptr;
    bool started_ = false;
    volatile bool suppressed_ = false;
};

#endif
