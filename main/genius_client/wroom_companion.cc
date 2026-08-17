#include "wroom_companion.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

constexpr uart_port_t kControlUart = UART_NUM_1;
constexpr gpio_num_t kControlTx = GPIO_NUM_18;  // Minji TX -> WROOM GPIO16 RX
constexpr gpio_num_t kControlRx = GPIO_NUM_3;   // Minji RX <- WROOM GPIO17 TX
constexpr int kControlBaud = 115200;
constexpr int kRxBufferSize = 512;
constexpr int64_t kPresenceTimeoutUs = 5LL * 1000LL * 1000LL;
constexpr const char* TAG = "WroomCompanion";

}  // namespace

WroomCompanion& WroomCompanion::GetInstance() {
    static WroomCompanion instance;
    return instance;
}

bool WroomCompanion::Initialize() {
    bool expected = false;
    if (!initialized_.compare_exchange_strong(expected, true)) {
        return true;
    }

    uart_config_t config = {};
    config.baud_rate = kControlBaud;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_driver_install(kControlUart, kRxBufferSize, 0, 0, nullptr, 0);
    if (err == ESP_OK) {
        err = uart_param_config(kControlUart, &config);
    }
    if (err == ESP_OK) {
        err = uart_set_pin(kControlUart, kControlTx, kControlRx,
                           UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Control UART initialization failed: %s", esp_err_to_name(err));
        initialized_ = false;
        return false;
    }

    if (xTaskCreate(TaskEntry, "wroom_control", 4096, this, 3, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "Unable to create WROOM control task");
        uart_driver_delete(kControlUart);
        initialized_ = false;
        return false;
    }

    ESP_LOGI(TAG, "Control UART ready: TX=G18 RX=G3 baud=115200");
    return true;
}

bool WroomCompanion::IsAvailable() const {
    if (!available_.load()) {
        return false;
    }
    return esp_timer_get_time() - last_seen_us_.load() <= kPresenceTimeoutUs;
}

bool WroomCompanion::IsBluetoothConnected() const {
    return IsAvailable() && bt_connected_.load();
}

std::string WroomCompanion::GetStatusText() const {
    if (!IsAvailable()) {
        return "Modul Bluetooth WROOM tidak terdeteksi. Media akan memakai speaker internal Minji.";
    }

    std::lock_guard<std::mutex> lock(state_mutex_);
    if (bt_connected_.load()) {
        char text[192];
        std::snprintf(text, sizeof(text),
                      "Bluetooth terhubung ke %s dengan volume %d persen.",
                      device_name_.c_str(), volume_.load());
        return text;
    }
    return "Modul WROOM terdeteksi, tetapi speaker Bluetooth tidak terhubung. Media akan memakai speaker internal Minji.";
}

bool WroomCompanion::SendLine(const char* line) {
    if (!initialized_.load() || line == nullptr) {
        return false;
    }
    std::lock_guard<std::mutex> lock(tx_mutex_);
    int expected = static_cast<int>(std::strlen(line));
    int written = uart_write_bytes(kControlUart, line, expected);
    written += uart_write_bytes(kControlUart, "\n", 1);
    return written == expected + 1;
}

bool WroomCompanion::RequestStatus() {
    return SendLine("BT STATUS");
}

bool WroomCompanion::Connect() {
    return SendLine("BT CONNECT");
}

bool WroomCompanion::Disconnect() {
    return SendLine("BT DISCONNECT");
}

bool WroomCompanion::SetVolume(int volume) {
    volume = std::clamp(volume, 0, 100);
    volume_ = volume;
    char command[32];
    std::snprintf(command, sizeof(command), "BT VOLUME %d", volume);
    return SendLine(command);
}

bool WroomCompanion::AdjustVolume(int delta) {
    return SetVolume(volume_.load() + delta);
}

std::string WroomCompanion::ScanDevices() {
    if (!IsAvailable()) {
        return "Modul Bluetooth WROOM tidak terdeteksi.";
    }

    {
        std::lock_guard<std::mutex> lock(scan_mutex_);
        scan_complete_ = false;
        scan_error_.clear();
        scan_devices_.clear();
    }
    if (!SendLine("BT SCAN")) {
        return "Perintah pemindaian Bluetooth gagal dikirim.";
    }

    std::unique_lock<std::mutex> lock(scan_mutex_);
    if (!scan_cv_.wait_for(lock, std::chrono::seconds(8), [this]() { return scan_complete_; })) {
        return "Pemindaian Bluetooth belum selesai. Silakan coba tanyakan kembali.";
    }
    if (!scan_error_.empty()) {
        return scan_error_;
    }
    if (scan_devices_.empty()) {
        return "Pemindaian selesai, tetapi tidak ada perangkat Bluetooth yang terlihat.";
    }

    std::string result = "Perangkat Bluetooth yang terlihat: ";
    for (size_t i = 0; i < scan_devices_.size(); ++i) {
        if (i > 0) {
            result += "; ";
        }
        result += std::to_string(i + 1) + ". " + scan_devices_[i];
    }
    return result + ".";
}

void WroomCompanion::TaskEntry(void* arg) {
    static_cast<WroomCompanion*>(arg)->Run();
    vTaskDelete(nullptr);
}

void WroomCompanion::HandleLine(const char* line) {
    if (line == nullptr || *line == '\0') {
        return;
    }
    ESP_LOGI(TAG, "RX: %s", line);

    if (std::strncmp(line, "WROOM ", 6) != 0 &&
        std::strncmp(line, "BT ", 3) != 0 &&
        std::strncmp(line, "OK ", 3) != 0 &&
        std::strncmp(line, "ERR ", 4) != 0 &&
        std::strncmp(line, "PONG ", 5) != 0) {
        return;
    }

    available_ = true;
    last_seen_us_ = esp_timer_get_time();

    if (std::strcmp(line, "BT SCAN BEGIN") == 0) {
        std::lock_guard<std::mutex> lock(scan_mutex_);
        scan_devices_.clear();
        return;
    }
    if (std::strncmp(line, "BT DEVICE ", 10) == 0) {
        const char* name = std::strstr(line, "NAME=\"");
        const char* address = std::strstr(line, "ADDR=");
        const char* rssi = std::strstr(line, "RSSI=");
        std::string device_name = "Perangkat tanpa nama";
        if (name != nullptr) {
            name += 6;
            const char* end = std::strchr(name, '"');
            if (end != nullptr && end > name && std::strncmp(name, "Unknown", 7) != 0) {
                device_name.assign(name, end - name);
            }
        }
        char detail[128];
        char parsed_address[18] = "unknown";
        int parsed_rssi = -129;
        if (address != nullptr) {
            std::sscanf(address + 5, "%17s", parsed_address);
        }
        if (rssi != nullptr) {
            std::sscanf(rssi + 5, "%d", &parsed_rssi);
        }
        std::snprintf(detail, sizeof(detail), "%s, sinyal %d dBm, alamat %s",
                      device_name.c_str(), parsed_rssi, parsed_address);
        std::lock_guard<std::mutex> lock(scan_mutex_);
        scan_devices_.emplace_back(detail);
        return;
    }
    if (std::strncmp(line, "BT SCAN END ", 12) == 0) {
        {
            std::lock_guard<std::mutex> lock(scan_mutex_);
            scan_complete_ = true;
        }
        scan_cv_.notify_all();
        return;
    }
    if (std::strncmp(line, "ERR BT SCAN ", 12) == 0) {
        {
            std::lock_guard<std::mutex> lock(scan_mutex_);
            scan_error_ = std::strstr(line, "BUSY_CONNECTED")
                ? "Speaker Bluetooth masih terhubung. Putuskan dahulu sebelum memindai perangkat sekitar."
                : "WROOM tidak dapat memulai pemindaian Bluetooth.";
            scan_complete_ = true;
        }
        scan_cv_.notify_all();
        return;
    }

    if (std::strncmp(line, "BT STATE ", 9) == 0) {
        const char* state_begin = line + 9;
        const char* state_end = std::strchr(state_begin, ' ');
        std::string parsed_state = state_end
            ? std::string(state_begin, state_end - state_begin)
            : std::string(state_begin);
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_ = parsed_state;
            const char* name = std::strstr(line, "NAME=\"");
            if (name != nullptr) {
                name += 6;
                const char* end = std::strchr(name, '"');
                if (end != nullptr && end > name) {
                    device_name_.assign(name, end - name);
                }
            }
        }
        bt_connected_ = parsed_state == "CONNECTED";
        avrc_connected_ = std::strstr(line, "AVRCP=1") != nullptr;
        const char* volume = std::strstr(line, "VOL=");
        if (volume != nullptr) {
            int parsed_volume = -1;
            if (std::sscanf(volume + 4, "%d", &parsed_volume) == 1 &&
                parsed_volume >= 0 && parsed_volume <= 100) {
                volume_ = parsed_volume;
            }
        }
    } else if (std::strcmp(line, "BT EVENT CONNECTED") == 0) {
        bt_connected_ = true;
    } else if (std::strcmp(line, "BT EVENT DISCONNECTED") == 0) {
        bt_connected_ = false;
        avrc_connected_ = false;
    } else if (std::strcmp(line, "BT EVENT AVRCP_CONNECTED") == 0) {
        avrc_connected_ = true;
    } else if (std::strcmp(line, "BT EVENT AVRCP_DISCONNECTED") == 0) {
        avrc_connected_ = false;
    }
}

void WroomCompanion::Run() {
    uint8_t data[64];
    char line[192];
    size_t used = 0;
    int64_t last_poll = 0;

    SendLine("PING");
    SendLine("BT STATUS");

    while (true) {
        int received = uart_read_bytes(kControlUart, data, sizeof(data), pdMS_TO_TICKS(100));
        for (int i = 0; i < received; ++i) {
            char ch = static_cast<char>(data[i]);
            if (ch == '\r' || ch == '\n') {
                if (used > 0) {
                    line[used] = '\0';
                    HandleLine(line);
                    used = 0;
                }
            } else if (used + 1 < sizeof(line)) {
                line[used++] = ch;
            } else {
                used = 0;
            }
        }

        int64_t now = esp_timer_get_time();
        if (now - last_poll >= 2LL * 1000LL * 1000LL) {
            SendLine("BT STATUS");
            last_poll = now;
        }
        if (available_.load() && now - last_seen_us_.load() > kPresenceTimeoutUs) {
            available_ = false;
            bt_connected_ = false;
            avrc_connected_ = false;
            ESP_LOGW(TAG, "WROOM status timeout; internal media fallback active");
        }
    }
}
