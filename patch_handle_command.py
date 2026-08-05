from pathlib import Path

path = Path(
    r"D:\xiaozhi-esp32\main\genius_client\genius_client.cc"
)

source = path.read_text(encoding="utf-8")

start_marker = "void GeniusClient::HandleCommand("
end_marker = "void GeniusClient::StartLocalAudio("

start = source.find(start_marker)
end = source.find(end_marker)

if start == -1:
    raise RuntimeError("HandleCommand tidak ditemukan")

if end == -1 or end <= start:
    raise RuntimeError("StartLocalAudio tidak ditemukan")

backup = path.with_suffix(".cc.before_command_fix")
backup.write_text(source, encoding="utf-8")

new_function = r'''void GeniusClient::HandleCommand(
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
        Application::GetInstance()
            .GetAudioService()
            .ResetDecoder();

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


'''

source = (
    source[:start]
    + new_function
    + source[end:]
)

if '#include "application.h"' not in source:
    source = source.replace(
        '#include "genius_client.h"',
        '#include "genius_client.h"\n#include "application.h"',
        1,
    )

path.write_text(source, encoding="utf-8")

print("HANDLE COMMAND BERHASIL DIPERBAIKI")
print("Backup:", backup)