#include "genius_client.h"

#include "boards/common/board.h"
#include "system_info.h"

#include <cJSON.h>
#include <esp_app_desc.h>
#include <esp_log.h>

#include <algorithm>
#include <cctype>
#include <utility>
#include <cstring>
#include "audio_stream_client.h"
#include "application.h"


namespace {

constexpr const char* TAG = "GeniusClient";

// Alamat PC GeniusAI Core di jaringan rumah.
constexpr const char* GENIUS_SERVER_URL =
    "http://192.168.1.11:8000";

constexpr uint32_t REGISTER_RETRY_MS = 10000;
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 30000;
constexpr uint32_t COMMAND_POLL_INTERVAL_MS = 2000;

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

        const TickType_t now = xTaskGetTickCount();

        if (
            last_heartbeat_tick == 0 ||
            now - last_heartbeat_tick >=
                pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS)
        ) {
            if (!SendHeartbeat()) {
                ESP_LOGW(TAG, "Heartbeat failed");
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

    // Volume sementara. Nanti dibaca dari audio codec asli.
    cJSON_AddNumberToObject(
        root,
        "volume",
        70
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
