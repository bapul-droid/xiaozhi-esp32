#ifndef WROOM_COMPANION_H
#define WROOM_COMPANION_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

class WroomCompanion {
public:
    static WroomCompanion& GetInstance();

    bool Initialize();
    bool IsAvailable() const;
    bool IsBluetoothConnected() const;
    int GetVolume() const { return volume_.load(); }
    std::string GetStatusText() const;

    bool RequestStatus();
    bool Connect();
    bool Disconnect();
    bool SetVolume(int volume);
    bool AdjustVolume(int delta);

private:
    WroomCompanion() = default;
    WroomCompanion(const WroomCompanion&) = delete;
    WroomCompanion& operator=(const WroomCompanion&) = delete;

    static void TaskEntry(void* arg);
    void Run();
    bool SendLine(const char* line);
    void HandleLine(const char* line);

    std::atomic<bool> initialized_{false};
    std::atomic<bool> available_{false};
    std::atomic<bool> bt_connected_{false};
    std::atomic<bool> avrc_connected_{false};
    std::atomic<int> volume_{70};
    std::atomic<int64_t> last_seen_us_{0};
    mutable std::mutex state_mutex_;
    std::mutex tx_mutex_;
    std::string state_ = "UNKNOWN";
    std::string device_name_ = "Edifier";
};

#endif

