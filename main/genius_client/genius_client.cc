#include "genius_client.h"

#include "boards/common/board.h"
#include "system_info.h"

#include <cJSON.h>
#include <esp_app_desc.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_attr.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <cstdio>

#include <algorithm>
#include <cctype>
#include <utility>
#include <cstring>
#include "audio_stream_client.h"
#include "application.h"
#include "audio_codec.h"


namespace {

constexpr const char* TAG = "GeniusClient";

// Alamat PC GeniusAI Core di jaringan rumah.
constexpr const char* GENIUS_SERVER_URL =
    "http://192.168.1.89:8000";

constexpr uint32_t REGISTER_RETRY_MS = 10000;
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 30000;
constexpr uint32_t COMMAND_POLL_INTERVAL_MS = 2000;

constexpr uint32_t DIAG_MAGIC = 0x4D494E4A;  // "MINJ"

constexpr gpio_num_t MINJI_BAT_ADC_GPIO = GPIO_NUM_11;
constexpr gpio_num_t MINJI_CHRG_GPIO = GPIO_NUM_12;
constexpr int MINJI_BAT_SAMPLES = 16;
constexpr int MINJI_CHARGING_THRESHOLD_MV = 500;

struct MinjiBatteryAdcState {
    adc_oneshot_unit_handle_t adc2 = nullptr;
    adc_cali_handle_t cali = nullptr;
    adc_channel_t bat_channel = ADC_CHANNEL_0;
    adc_channel_t chrg_channel = ADC_CHANNEL_1;
    bool initialized = false;
    bool calibration_ok = false;
};

MinjiBatteryAdcState g_battery_adc;

bool InitMinjiBatteryAdc()
{
    if (g_battery_adc.initialized) {
        return g_battery_adc.adc2 != nullptr;
    }

    g_battery_adc.initialized = true;

    adc_unit_t bat_unit = ADC_UNIT_2;
    adc_unit_t chrg_unit = ADC_UNIT_2;

    esp_err_t err = adc_oneshot_io_to_channel(
        MINJI_BAT_ADC_GPIO,
        &bat_unit,
        &g_battery_adc.bat_channel
    );
    if (err != ESP_OK || bat_unit != ADC_UNIT_2) {
        ESP_LOGE(TAG, "GPIO11 BAT_ADC mapping failed: %s", esp_err_to_name(err));
        return false;
    }

    err = adc_oneshot_io_to_channel(
        MINJI_CHRG_GPIO,
        &chrg_unit,
        &g_battery_adc.chrg_channel
    );
    if (err != ESP_OK || chrg_unit != ADC_UNIT_2) {
        ESP_LOGE(TAG, "GPIO12 CHRG mapping failed: %s", esp_err_to_name(err));
        return false;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg{};
    unit_cfg.unit_id = ADC_UNIT_2;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

    err = adc_oneshot_new_unit(&unit_cfg, &g_battery_adc.adc2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC2 init failed: %s", esp_err_to_name(err));
        g_battery_adc.adc2 = nullptr;
        return false;
    }

    adc_oneshot_chan_cfg_t chan_cfg{};
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    chan_cfg.atten = ADC_ATTEN_DB_12;

    err = adc_oneshot_config_channel(
        g_battery_adc.adc2,
        g_battery_adc.bat_channel,
        &chan_cfg
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO11 ADC config failed: %s", esp_err_to_name(err));
        return false;
    }

    err = adc_oneshot_config_channel(
        g_battery_adc.adc2,
        g_battery_adc.chrg_channel,
        &chan_cfg
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO12 ADC config failed: %s", esp_err_to_name(err));
        return false;
    }

    adc_cali_curve_fitting_config_t cali_cfg{};
    cali_cfg.unit_id = ADC_UNIT_2;
    cali_cfg.atten = ADC_ATTEN_DB_12;
    cali_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;

    err = adc_cali_create_scheme_curve_fitting(
        &cali_cfg,
        &g_battery_adc.cali
    );
    if (err == ESP_OK) {
        g_battery_adc.calibration_ok = true;
    } else {
        ESP_LOGW(
            TAG,
            "ADC2 calibration unavailable (%s); raw values still usable",
            esp_err_to_name(err)
        );
    }

    ESP_LOGI(
        TAG,
        "Minji battery ADC ready: GPIO11=BAT_ADC, GPIO12=CHRG"
    );

    return true;
}

bool ReadMinjiAdcChannel(
    adc_channel_t channel,
    int& raw_out,
    int& mv_out
)
{
    raw_out = -1;
    mv_out = -1;

    if (!InitMinjiBatteryAdc()) {
        return false;
    }

    int64_t raw_sum = 0;
    int good_samples = 0;

    for (int i = 0; i < MINJI_BAT_SAMPLES; ++i) {
        int raw = 0;
        const esp_err_t err = adc_oneshot_read(
            g_battery_adc.adc2,
            channel,
            &raw
        );

        if (err == ESP_OK) {
            raw_sum += raw;
            ++good_samples;
        }
    }

    if (good_samples == 0) {
        return false;
    }

    raw_out = static_cast<int>(raw_sum / good_samples);

    if (g_battery_adc.calibration_ok && g_battery_adc.cali != nullptr) {
        int mv = 0;
        if (
            adc_cali_raw_to_voltage(
                g_battery_adc.cali,
                raw_out,
                &mv
            ) == ESP_OK
        ) {
            mv_out = mv;
        }
    }

    return true;
}


struct DiagnosticRtcState {
    uint32_t magic;
    int32_t last_state;
    char last_event[48];
};

RTC_NOINIT_ATTR DiagnosticRtcState g_diag_rtc;

void EnsureDiagnosticRtcInitialized()
{
    if (g_diag_rtc.magic != DIAG_MAGIC) {
        g_diag_rtc.magic = DIAG_MAGIC;
        g_diag_rtc.last_state = -1;
        std::snprintf(g_diag_rtc.last_event, sizeof(g_diag_rtc.last_event), "%s", "unknown");
    }
}

const char* ResetReasonToString(esp_reset_reason_t reason)
{
    switch (reason) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "EXTERNAL";
        case ESP_RST_SW:        return "SOFTWARE";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT_WDT";
        case ESP_RST_TASK_WDT:  return "TASK_WDT";
        case ESP_RST_WDT:       return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        case ESP_RST_USB:       return "USB";
        case ESP_RST_JTAG:      return "JTAG";
        case ESP_RST_EFUSE:     return "EFUSE";
        case ESP_RST_PWR_GLITCH:return "POWER_GLITCH";
        case ESP_RST_CPU_LOCKUP:return "CPU_LOCKUP";
        default:                return "UNKNOWN";
    }
}

}  // namespace

struct LocalAudioTaskArgs {
    GeniusClient* client;
    std::string filename;
};
GeniusClient& GeniusClient::GetInstance()
{
    static GeniusClient instance;
    return instance;
}


void GeniusClient::RecordDiagnosticState(int state)
{
    EnsureDiagnosticRtcInitialized();
    g_diag_rtc.last_state = state;
}

void GeniusClient::RecordDiagnosticEvent(const char* event)
{
    EnsureDiagnosticRtcInitialized();
    std::snprintf(
        g_diag_rtc.last_event,
        sizeof(g_diag_rtc.last_event),
        "%s",
        event != nullptr ? event : "null"
    );
}


void GeniusClient::Start()
{
    // Jangan membuat task kedua saat Wi-Fi reconnect.
    if (task_handle_ != nullptr) {
        ESP_LOGI(TAG, "Genius Client task already running");
        return;
    }

    BaseType_t result = xTaskCreate(
        TaskEntry,
        "genius_client",
        6144,
        this,
        2,
        &task_handle_
    );

    if (result != pdPASS) {
        task_handle_ = nullptr;
        ESP_LOGE(TAG, "Failed to create Genius Client task");
        return;
    }

    ESP_LOGI(TAG, "Genius Client task started");
}


void GeniusClient::TaskEntry(void* arg)
{
    auto* client = static_cast<GeniusClient*>(arg);

    client->Run();

    client->task_handle_ = nullptr;
    vTaskDelete(nullptr);
}


void GeniusClient::Run()
{
    TickType_t last_heartbeat_tick = 0;

    while (true) {
        if (!registered_) {
            registered_ = RegisterDevice();

            if (!registered_) {
                ESP_LOGW(
                    TAG,
                    "Registration failed, retrying in %lu ms",
                    static_cast<unsigned long>(REGISTER_RETRY_MS)
                );

                vTaskDelay(
                    pdMS_TO_TICKS(REGISTER_RETRY_MS)
                );

                continue;
            }

            last_heartbeat_tick = 0;
        }

        if (!boot_report_sent_) {
            boot_report_sent_ = SendBootCrashReport();
            if (!boot_report_sent_) {
                ESP_LOGW(TAG, "Boot crash report failed; will retry");
            }
        }

        const TickType_t now = xTaskGetTickCount();

        if (
            last_heartbeat_tick == 0 ||
            now - last_heartbeat_tick >=
                pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS)
        ) {
            if (!SendHeartbeat()) {
                ESP_LOGW(TAG, "Heartbeat failed");
            }

            if (!SendBatteryTelemetry()) {
                ESP_LOGW(TAG, "Battery telemetry failed");
            }

            last_heartbeat_tick = now;
        }

        // Keep polling device commands while media is playing so
        // stop_audio can be received during radio/music playback.
        FetchNextCommand();

        vTaskDelay(
            pdMS_TO_TICKS(COMMAND_POLL_INTERVAL_MS)
        );
    }
}

std::string GeniusClient::BuildDeviceId() const
{
    std::string mac = SystemInfo::GetMacAddress();

    mac.erase(
        std::remove(mac.begin(), mac.end(), ':'),
        mac.end()
    );

    std::transform(
        mac.begin(),
        mac.end(),
        mac.begin(),
        [](unsigned char character) {
            return static_cast<char>(
                std::tolower(character)
            );
        }
    );

    // Contoh: genius-e83dc1f1bf34
    return "genius-" + mac;
}


bool GeniusClient::RegisterDevice()
{
    const std::string device_id = BuildDeviceId();
    const std::string mac_address =
        SystemInfo::GetMacAddress();

    const esp_app_desc_t* app_description =
        esp_app_get_description();

    cJSON* root = cJSON_CreateObject();

    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to create registration JSON");
        return false;
    }

    cJSON_AddStringToObject(
        root,
        "device_id",
        device_id.c_str()
    );

    cJSON_AddStringToObject(
        root,
        "mac_address",
        mac_address.c_str()
    );

    cJSON_AddStringToObject(
        root,
        "name",
        "Genius Ruang Tamu"
    );

    cJSON_AddStringToObject(
        root,
        "board",
        BOARD_NAME
    );

    cJSON_AddStringToObject(
        root,
        "firmware_version",
        app_description->version
    );

    char* json_text =
        cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    if (json_text == nullptr) {
        ESP_LOGE(TAG, "Failed to serialize registration JSON");
        return false;
    }

    std::string json_body(json_text);
    cJSON_free(json_text);

    const bool success = PostJson(
        "/api/devices/register",
        json_body
    );

    if (success) {
        ESP_LOGI(
            TAG,
            "Device registered: %s",
            device_id.c_str()
        );
    }

    return success;
}


bool GeniusClient::SendBootCrashReport()
{
    EnsureDiagnosticRtcInitialized();

    const esp_reset_reason_t reset_reason = esp_reset_reason();

    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        return false;
    }

    cJSON_AddStringToObject(root, "device_id", BuildDeviceId().c_str());
    cJSON_AddStringToObject(root, "reset_reason", ResetReasonToString(reset_reason));
    cJSON_AddNumberToObject(root, "reset_reason_code", static_cast<int>(reset_reason));
    cJSON_AddNumberToObject(root, "uptime_ms", static_cast<double>(esp_timer_get_time() / 1000));
    cJSON_AddNumberToObject(root, "last_state", g_diag_rtc.last_state);
    cJSON_AddStringToObject(root, "last_event", g_diag_rtc.last_event);
    cJSON_AddNumberToObject(root, "free_sram", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "minimum_sram", esp_get_minimum_free_heap_size());

    char* json_text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_text == nullptr) {
        return false;
    }

    std::string json_body(json_text);
    cJSON_free(json_text);

    const bool success = PostJson("/api/console/crash/report", json_body);
    if (success) {
        ESP_LOGI(
            TAG,
            "Boot report sent: reset=%s, last_state=%ld, last_event=%s",
            ResetReasonToString(reset_reason),
            static_cast<long>(g_diag_rtc.last_state),
            g_diag_rtc.last_event
        );

        // Marker boot baru. Jika crash terjadi sebelum event lain tercatat,
        // server tetap dapat melihat bahwa firmware sudah berhasil boot.
        g_diag_rtc.last_state = -1;
        std::snprintf(g_diag_rtc.last_event, sizeof(g_diag_rtc.last_event), "%s", "booted");
    }

    return success;
}


bool GeniusClient::SendHeartbeat()
{
    const std::string device_id = BuildDeviceId();

    cJSON* root = cJSON_CreateObject();

    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to create heartbeat JSON");
        return false;
    }

    cJSON_AddStringToObject(
        root,
        "device_id",
        device_id.c_str()
    );

    cJSON_AddStringToObject(
        root,
        "state",
        "idle"
    );

    // Kirim volume speaker aktual dari AudioCodec.
    int volume = 0;
    if (auto* codec = Board::GetInstance().GetAudioCodec(); codec != nullptr) {
        volume = codec->output_volume();
    }

    cJSON_AddNumberToObject(
        root,
        "volume",
        volume
    );

    char* json_text =
        cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    if (json_text == nullptr) {
        ESP_LOGE(TAG, "Failed to serialize heartbeat JSON");
        return false;
    }

    std::string json_body(json_text);
    cJSON_free(json_text);

    const bool success = PostJson(
        "/api/devices/heartbeat",
        json_body
    );

    if (success) {
        ESP_LOGI(TAG, "Heartbeat sent successfully");
    }

    return success;
}



bool GeniusClient::SendBatteryTelemetry()
{
    int bat_raw = -1;
    int bat_mv = -1;
    int chrg_raw = -1;
    int chrg_mv = -1;

    const bool bat_ok = ReadMinjiAdcChannel(
        g_battery_adc.bat_channel,
        bat_raw,
        bat_mv
    );

    const bool chrg_ok = ReadMinjiAdcChannel(
        g_battery_adc.chrg_channel,
        chrg_raw,
        chrg_mv
    );

    if (!bat_ok || !chrg_ok) {
        ESP_LOGW(
            TAG,
            "Battery ADC read failed: BAT=%s CHRG=%s",
            bat_ok ? "ok" : "fail",
            chrg_ok ? "ok" : "fail"
        );
        return false;
    }

    cJSON* root = cJSON_CreateObject();
    cJSON* pins = cJSON_CreateArray();

    if (root == nullptr || pins == nullptr) {
        if (root != nullptr) {
            cJSON_Delete(root);
        }
        if (pins != nullptr) {
            cJSON_Delete(pins);
        }
        return false;
    }

    cJSON_AddStringToObject(root, "tool", "Minji Battery Monitor");
    cJSON_AddStringToObject(root, "version", "1.0");
    cJSON_AddStringToObject(root, "target", "ESP32-S3");
    cJSON_AddStringToObject(root, "device_id", BuildDeviceId().c_str());
    cJSON_AddNumberToObject(
        root,
        "uptime_ms",
        static_cast<double>(esp_timer_get_time() / 1000)
    );

    auto add_pin = [pins](
        int gpio,
        int adc,
        int channel,
        int raw,
        int mv
    ) {
        cJSON* item = cJSON_CreateObject();
        if (item == nullptr) {
            return;
        }

        cJSON_AddNumberToObject(item, "gpio", gpio);
        cJSON_AddNumberToObject(item, "adc", adc);
        cJSON_AddNumberToObject(item, "channel", channel);
        cJSON_AddBoolToObject(item, "ok", true);
        cJSON_AddNumberToObject(item, "raw", raw);
        cJSON_AddNumberToObject(item, "mv", mv);

        cJSON_AddItemToArray(pins, item);
    };

    add_pin(
        static_cast<int>(MINJI_BAT_ADC_GPIO),
        2,
        static_cast<int>(g_battery_adc.bat_channel),
        bat_raw,
        bat_mv
    );

    add_pin(
        static_cast<int>(MINJI_CHRG_GPIO),
        2,
        static_cast<int>(g_battery_adc.chrg_channel),
        chrg_raw,
        chrg_mv
    );

    cJSON_AddItemToObject(root, "pins", pins);

    const bool charging =
        chrg_mv >= 0 &&
        chrg_mv < MINJI_CHARGING_THRESHOLD_MV;

    cJSON_AddBoolToObject(root, "charging", charging);

    char* json_text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_text == nullptr) {
        return false;
    }

    const std::string json_body(json_text);
    cJSON_free(json_text);

    ESP_LOGI(
        TAG,
        "Battery telemetry: BAT raw=%d mv=%d, CHRG raw=%d mv=%d, charging=%s",
        bat_raw,
        bat_mv,
        chrg_raw,
        chrg_mv,
        charging ? "yes" : "no"
    );

    return PostJson(
        "/api/debug/adc",
        json_body
    );
}

bool GeniusClient::PostJson(
    const std::string& endpoint,
    const std::string& json_body
)
{
    auto network =
        Board::GetInstance().GetNetwork();

    if (network == nullptr) {
        MarkServerUnavailable();
        ESP_LOGE(TAG, "Network interface is unavailable");
        return false;
    }

    auto http = network->CreateHttp(3);

    if (http == nullptr) {
        MarkServerUnavailable();
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    const std::string url =
        std::string(GENIUS_SERVER_URL) + endpoint;

    http->SetHeader(
        "Content-Type",
        "application/json"
    );

    http->SetHeader(
        "User-Agent",
        "GeniusAI-ESP32/0.1"
    );

    http->SetContent(
        std::string(json_body)
    );

    ESP_LOGI(TAG, "POST %s", url.c_str());

    if (!http->Open("POST", url)) {
        MarkServerUnavailable();
        ESP_LOGE(
            TAG,
            "Failed to connect to GeniusAI Core, error=0x%x",
            http->GetLastError()
        );

        return false;
    }

    MarkServerAvailable();

    const int status_code =
        http->GetStatusCode();

    const std::string response_body =
        http->ReadAll();

    http->Close();

    if (status_code < 200 || status_code >= 300) {
        ESP_LOGE(
            TAG,
            "HTTP %d: %s",
            status_code,
            response_body.c_str()
        );

        return false;
    }

    ESP_LOGI(
        TAG,
        "HTTP %d: %s",
        status_code,
        response_body.c_str()
    );

    return true;
}

bool GeniusClient::GetJson(
    const std::string& endpoint,
    std::string& response_body
)
{
    auto network =
        Board::GetInstance().GetNetwork();

    if (network == nullptr) {
        MarkServerUnavailable();
        ESP_LOGE(TAG, "Network interface is unavailable");
        return false;
    }

    auto http = network->CreateHttp(3);

    if (http == nullptr) {
        MarkServerUnavailable();
        ESP_LOGE(TAG, "Failed to create HTTP client");
        return false;
    }

    const std::string url =
        std::string(GENIUS_SERVER_URL) + endpoint;

    http->SetHeader(
        "Accept",
        "application/json"
    );

    http->SetHeader(
        "User-Agent",
        "GeniusAI-ESP32/0.1"
    );

    if (!http->Open("GET", url)) {
        MarkServerUnavailable();
        ESP_LOGW(
            TAG,
            "GET failed: %s, error=0x%x",
            url.c_str(),
            http->GetLastError()
        );

        return false;
    }

    MarkServerAvailable();

    const int status_code =
        http->GetStatusCode();

    response_body = http->ReadAll();
    http->Close();

    if (status_code < 200 || status_code >= 300) {
        ESP_LOGW(
            TAG,
            "GET HTTP %d: %s",
            status_code,
            response_body.c_str()
        );

        return false;
    }

    return true;
}

bool GeniusClient::FetchNextCommand()
{
    const std::string endpoint =
        "/api/devices/" +
        BuildDeviceId() +
        "/commands/next";

    std::string response_body;

    if (!GetJson(endpoint, response_body)) {
        return false;
    }

    HandleCommand(response_body);
    return true;
}
void GeniusClient::HandleCommand(
    const std::string& response_body
)
{
    cJSON* root =
        cJSON_Parse(response_body.c_str());

    if (root == nullptr) {
        ESP_LOGW(
            TAG,
            "Invalid command JSON: %s",
            response_body.c_str()
        );
        return;
    }

    cJSON* command =
        cJSON_GetObjectItem(root, "command");

    if (
        command == nullptr ||
        cJSON_IsNull(command)
    ) {
        cJSON_Delete(root);
        return;
    }

    cJSON* command_id =
        cJSON_GetObjectItem(command, "command_id");

    cJSON* action =
        cJSON_GetObjectItem(command, "action");

    cJSON* payload =
        cJSON_GetObjectItem(command, "payload");

    if (!cJSON_IsString(action)) {
        ESP_LOGW(TAG, "Command action is missing");
        cJSON_Delete(root);
        return;
    }

    const char* action_text =
        action->valuestring;

    ESP_LOGI(
        TAG,
        "Command received: id=%s, action=%s",
        cJSON_IsString(command_id)
            ? command_id->valuestring
            : "unknown",
        action_text
    );

    if (strcmp(action_text, "notify") == 0) {
        cJSON* message = payload
            ? cJSON_GetObjectItem(payload, "message")
            : nullptr;

        if (cJSON_IsString(message)) {
            ESP_LOGI(
                TAG,
                "Notification: %s",
                message->valuestring
            );
        } else {
            ESP_LOGW(
                TAG,
                "Notify command has no message"
            );
        }

    } else if (
        strcmp(action_text, "play_local") == 0
    ) {
        cJSON* track = payload
            ? cJSON_GetObjectItem(payload, "track")
            : nullptr;

        if (!cJSON_IsString(track)) {
            ESP_LOGW(
                TAG,
                "play_local command has no track"
            );
        } else {
            StartLocalAudio(track->valuestring);
        }

    } else if (
        strcmp(action_text, "stop_audio") == 0
    ) {
        StopAudio();

        ESP_LOGI(TAG, "Audio stop requested");

    } else {
        ESP_LOGW(
            TAG,
            "Unsupported command: %s",
            action_text
        );
    }

    cJSON_Delete(root);
}




void GeniusClient::StopAudio()
{
    if (audio_task_handle_ == nullptr) {
        ESP_LOGI(TAG, "No media playback to stop");

        Application::GetInstance()
            .GetAudioService()
            .ResetDecoder();

        return;
    }

    audio_stop_requested_.store(true);

    Application::GetInstance()
        .GetAudioService()
        .ResetDecoder();

    ESP_LOGI(TAG, "Audio stop requested");
}
bool GeniusClient::IsAudioPlaying() const
{
    return audio_task_handle_ != nullptr;
}

bool GeniusClient::GetNewsBulletin(
    const std::string& category,
    int limit,
    std::string& bulletin
)
{
    std::string safe_category =
        category.empty() ? "terkini" : category;

    if (limit < 1) {
        limit = 1;
    } else if (limit > 5) {
        limit = 5;
    }

    const std::string endpoint =
        "/api/news/presenter?category=" +
        safe_category +
        "&limit=" +
        std::to_string(limit);

    std::string response_body;

    ESP_LOGI(
        TAG,
        "News bulletin requested: category=%s limit=%d",
        safe_category.c_str(),
        limit
    );

    if (!GetJson(endpoint, response_body)) {
        ESP_LOGE(TAG, "Failed to fetch news bulletin");
        return false;
    }

    cJSON* root =
        cJSON_Parse(response_body.c_str());

    if (root == nullptr) {
        ESP_LOGE(TAG, "Invalid news JSON");
        return false;
    }

    cJSON* text =
        cJSON_GetObjectItem(root, "bulletin_text");

    if (!cJSON_IsString(text)) {
        ESP_LOGE(TAG, "News response has no bulletin_text");
        cJSON_Delete(root);
        return false;
    }

    bulletin = text->valuestring;

    cJSON_Delete(root);

    ESP_LOGI(
        TAG,
        "News bulletin ready: %u chars",
        static_cast<unsigned>(bulletin.size())
    );

    return true;
}
void GeniusClient::MarkServerAvailable()
{
    last_server_success_us_.store(esp_timer_get_time());
    server_available_.store(true);
}

void GeniusClient::MarkServerUnavailable()
{
    server_available_.store(false);
}

bool GeniusClient::IsServerAvailable()
{
    if (!server_available_.load()) {
        return false;
    }

    constexpr int64_t kServerStateTtlUs = 5LL * 1000LL * 1000LL;
    const int64_t last_success = last_server_success_us_.load();

    if (last_success <= 0) {
        return false;
    }

    const int64_t age = esp_timer_get_time() - last_success;
    return age >= 0 && age <= kServerStateTtlUs;
}


bool GeniusClient::SearchKnowledge(
    const std::string& query,
    int limit,
    std::string& result
)
{
    if (query.empty()) {
        ESP_LOGW(TAG, "Knowledge query is empty");
        return false;
    }

    if (limit < 1) {
        limit = 1;
    } else if (limit > 5) {
        limit = 5;
    }

    std::string encoded_query;

    for (unsigned char ch : query) {
        if (
            std::isalnum(ch) ||
            ch == '-' ||
            ch == '_' ||
            ch == '.' ||
            ch == '~'
        ) {
            encoded_query += static_cast<char>(ch);
        } else if (ch == ' ') {
            encoded_query += "%20";
        } else {
            char buffer[4];
            snprintf(
                buffer,
                sizeof(buffer),
                "%%%02X",
                ch
            );
            encoded_query += buffer;
        }
    }

    const std::string endpoint =
        "/api/knowledge/search?q=" +
        encoded_query +
        "&limit=" +
        std::to_string(limit);

    std::string response_body;

    ESP_LOGI(
        TAG,
        "Knowledge search: %s",
        query.c_str()
    );

    if (!GetJson(endpoint, response_body)) {
        ESP_LOGW(TAG, "Knowledge server unavailable");
        return false;
    }

    cJSON* root =
        cJSON_Parse(response_body.c_str());

    if (root == nullptr) {
        ESP_LOGE(TAG, "Invalid knowledge JSON");
        return false;
    }

    cJSON* results =
        cJSON_GetObjectItem(root, "results");

    if (!cJSON_IsArray(results)) {
        ESP_LOGE(TAG, "Knowledge response has no results");
        cJSON_Delete(root);
        return false;
    }

    std::string text;

    cJSON* item = nullptr;

    cJSON_ArrayForEach(item, results) {
        cJSON* title =
            cJSON_GetObjectItem(item, "title");

        cJSON* snippet =
            cJSON_GetObjectItem(item, "snippet");

        cJSON* url =
            cJSON_GetObjectItem(item, "url");

        if (cJSON_IsString(title)) {
            text += "Sumber: ";
            text += title->valuestring;
            text += ". ";
        }

        if (cJSON_IsString(snippet)) {
            text += snippet->valuestring;
            text += " ";
        }

        if (cJSON_IsString(url)) {
            text += "URL: ";
            text += url->valuestring;
            text += ". ";
        }
    }

    cJSON_Delete(root);

    if (text.empty()) {
        ESP_LOGW(TAG, "Knowledge search returned no usable text");
        return false;
    }

    result = std::move(text);

    ESP_LOGI(
        TAG,
        "Knowledge result ready: %u chars",
        static_cast<unsigned>(result.size())
    );

    return true;
}
bool GeniusClient::PlayOnlineMusic(
    const std::string& query
)
{
    if (query.empty()) {
        ESP_LOGW(TAG, "Online music query is empty");
        return false;
    }

    cJSON* root = cJSON_CreateObject();

    if (root == nullptr) {
        ESP_LOGE(
            TAG,
            "Failed to create online music JSON"
        );
        return false;
    }

    cJSON_AddStringToObject(
        root,
        "query",
        query.c_str()
    );

    char* json_text =
        cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    if (json_text == nullptr) {
        ESP_LOGE(
            TAG,
            "Failed to serialize online music JSON"
        );
        return false;
    }

    const std::string json_body(json_text);
    cJSON_free(json_text);

    ESP_LOGI(
        TAG,
        "Online music search requested: %s",
        query.c_str()
    );

    if (!PostJson(
            "/api/music/search-play",
            json_body
        )) {
        ESP_LOGE(
            TAG,
            "Online music search failed: %s",
            query.c_str()
        );
        return false;
    }

    ESP_LOGI(
        TAG,
        "Online music prepared: %s",
        query.c_str()
    );

    PlayRadio("music");

    return true;
}
void GeniusClient::PlayRadio(
    const std::string& station_id
)
{
    if (audio_task_handle_ != nullptr) {
        ESP_LOGW(TAG, "Audio playback is already running");
        return;
    }

    audio_stop_requested_.store(false);

    auto* args = new LocalAudioTaskArgs{
        this,
        std::string("__radio__:") + station_id,
    };

    BaseType_t result = xTaskCreate(
        AudioTaskEntry,
        "genius_audio",
        12288,
        args,
        3,
        &audio_task_handle_
    );

    if (result != pdPASS) {
        audio_task_handle_ = nullptr;
        delete args;
        ESP_LOGE(TAG, "Failed to create radio playback task");
    }
}

void GeniusClient::StartLocalAudio(
    const std::string& filename
)
{
    ESP_LOGI(
        TAG,
        "StartLocalAudio(): %s",
        filename.c_str()
    );

    if (audio_task_handle_ != nullptr) {
        ESP_LOGW(
            TAG,
            "Audio playback is already running"
        );
        return;
    }

    audio_stop_requested_.store(false);

    auto* args = new LocalAudioTaskArgs{
        this,
        filename,
    };

    BaseType_t result = xTaskCreate(
        AudioTaskEntry,
        "genius_audio",
        12288,
        args,
        3,
        &audio_task_handle_
    );

    if (result != pdPASS) {
        audio_task_handle_ = nullptr;
        delete args;

        ESP_LOGE(
            TAG,
            "Failed to create audio playback task"
        );
    }
}


void GeniusClient::AudioTaskEntry(void* arg)
{
    auto* args =
        static_cast<LocalAudioTaskArgs*>(arg);

    GeniusClient* client = args->client;
    std::string filename = args->filename;

    delete args;

    ESP_LOGI(
        TAG,
        "Audio task started: %s",
        filename.c_str()
    );

    std::string url;

    const std::string radio_prefix = "__radio__:";

    if (filename.rfind(radio_prefix, 0) == 0) {
        const std::string station_id =
            filename.substr(radio_prefix.size());

        url =
            std::string(GENIUS_SERVER_URL) +
            "/api/radio-stream/" +
            station_id;
    } else {
        url =
            std::string(GENIUS_SERVER_URL) +
            "/api/audio/local/" +
            filename;
    }

    AudioStreamClient audio_client;

    const bool success =
        audio_client.Download(
            url,
            client->audio_stop_requested_
        );

    const bool was_stopped =
        client->audio_stop_requested_.load();

    ESP_LOGI(
        TAG,
        "Audio playback finished: %s",
        was_stopped
            ? "stopped"
            : (success ? "success" : "failed")
    );

    client->audio_task_handle_ = nullptr;

    vTaskDelete(nullptr);
}
