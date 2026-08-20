#ifndef GENIUS_CLIENT_H
#define GENIUS_CLIENT_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include "audio_stream_client.h"

#include <string>
#include <atomic>

class GeniusClient {
public:
    static GeniusClient& GetInstance();

    // Aman dipanggil kembali setelah Wi-Fi reconnect.
    void Start();

    // Crash Recorder v1: simpan marker ringan di RTC memory agar boot berikutnya
    // dapat melaporkan aktivitas terakhir sebelum reset/crash.
    void RecordDiagnosticState(int state);
    void RecordDiagnosticEvent(const char* event);

    // Dipanggil oleh MCP tool untuk memutar musik lokal.
    void PlayLocal(
        const std::string& filename
    ) {
        StartLocalAudio(filename);
    }

    void StopAudio();
    bool IsAudioPlaying() const;

void PlayRadio(
    const std::string& station_id
);

bool PlayOnlineMusic(
    const std::string& query
);
bool GetNewsBulletin(
    const std::string& category,
    int limit,
    std::string& bulletin
);
bool GetBatteryStatus(
    std::string& result
);
bool SearchKnowledge(
    const std::string& query,
    int limit,
    std::string& result
);

bool GetBardiStatus(
    std::string& result
);

bool SetBardiSwitch(
    const std::string& room,
    bool state
);
bool IsServerAvailable();
private:
    GeniusClient() = default;
    GeniusClient(const GeniusClient&) = delete;
    GeniusClient& operator=(const GeniusClient&) = delete;

    static void TaskEntry(void* arg);
    void Run();
    static void AudioTaskEntry(void* arg);

    void StartLocalAudio(
        const std::string& filename
    );
    bool RegisterDevice();
    bool SendHeartbeat();
    bool SendBatteryTelemetry();
    bool SendBootCrashReport();
    bool SendRemoteDeviceLogs();
    bool FetchNextCommand();
    bool GetJson(
        const std::string& endpoint,
        std::string& response_body
    );

    void HandleCommand(
        const std::string& response_body
    );

    bool PostJson(
        const std::string& endpoint,
        const std::string& json_body
    );

    void MarkServerAvailable();
    void MarkServerUnavailable();

    std::string BuildDeviceId() const;
    TaskHandle_t audio_task_handle_ = nullptr;
    TaskHandle_t task_handle_ = nullptr;

    std::atomic<bool> audio_stop_requested_{false};
    std::atomic<bool> server_available_{false};
    std::atomic<int64_t> last_server_success_us_{0};

    bool registered_ = false;
    bool boot_report_sent_ = false;
};

#endif