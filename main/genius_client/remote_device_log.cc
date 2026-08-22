#include "remote_device_log.h"

#include <cJSON.h>
#include <esp_log.h>
#include <esp_timer.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {
constexpr size_t REMOTE_LOG_QUEUE_LENGTH = 32;
vprintf_like_t g_previous_vprintf = nullptr;

void CopyText(char* dst, size_t dst_size, const char* src)
{
    if (dst == nullptr || dst_size == 0) return;

    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }

    size_t i = 0;
    while (i + 1 < dst_size && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

void TrimLine(char* text)
{
    if (text == nullptr) return;
    size_t len = std::strlen(text);
    while (len > 0 && (text[len - 1] == '\r' || text[len - 1] == '\n' || std::isspace(static_cast<unsigned char>(text[len - 1])))) {
        text[--len] = '\0';
    }
}

// Buang ANSI color escape agar parser menerima "I (123) TAG: message".
void StripAnsi(const char* src, char* dst, size_t dst_size)
{
    if (dst_size == 0) return;
    size_t out = 0;
    for (size_t i = 0; src != nullptr && src[i] != '\0' && out + 1 < dst_size; ++i) {
        if (static_cast<unsigned char>(src[i]) == 0x1b && src[i + 1] == '[') {
            i += 2;
            while (src[i] != '\0' && !std::isalpha(static_cast<unsigned char>(src[i]))) ++i;
            continue;
        }
        dst[out++] = src[i];
    }
    dst[out] = '\0';
}
}

RemoteDeviceLog& RemoteDeviceLog::GetInstance()
{
    static RemoteDeviceLog instance;
    return instance;
}

void RemoteDeviceLog::Start()
{
    if (started_) return;

    queue_ = xQueueCreate(REMOTE_LOG_QUEUE_LENGTH, sizeof(Entry));
    if (queue_ == nullptr) return;

    started_ = true;
    g_previous_vprintf = esp_log_set_vprintf(&RemoteDeviceLog::LogVprintf);
}

void RemoteDeviceLog::SetSuppressed(bool suppressed)
{
    suppressed_ = suppressed;
}

int RemoteDeviceLog::LogVprintf(const char* format, va_list args)
{
    char line[320];

    va_list copy_for_capture;
    va_copy(copy_for_capture, args);
    std::vsnprintf(line, sizeof(line), format, copy_for_capture);
    va_end(copy_for_capture);

    auto& self = RemoteDeviceLog::GetInstance();
    if (!self.suppressed_) self.EnqueueFormatted(line);

    if (g_previous_vprintf != nullptr) {
        va_list copy_for_console;
        va_copy(copy_for_console, args);
        const int result = g_previous_vprintf(format, copy_for_console);
        va_end(copy_for_console);
        return result;
    }

    va_list copy_for_console;
    va_copy(copy_for_console, args);
    const int result = std::vprintf(format, copy_for_console);
    va_end(copy_for_console);
    return result;
}

void RemoteDeviceLog::EnqueueFormatted(const char* raw_line)
{
    if (queue_ == nullptr || raw_line == nullptr) return;

    char line[320];
    StripAnsi(raw_line, line, sizeof(line));
    TrimLine(line);
    if (line[0] == '\0') return;

    Entry entry{};
    entry.uptime_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    CopyText(entry.level, sizeof(entry.level), "I");
    CopyText(entry.tag, sizeof(entry.tag), "ESP");

    const char* message = line;

    // Format ESP-IDF lazim: "I (12345) TAG: message".
    if ((line[0] == 'E' || line[0] == 'W' || line[0] == 'I' || line[0] == 'D' || line[0] == 'V') && line[1] == ' ') {
        entry.level[0] = line[0];
        entry.level[1] = '\0';

        const char* close = std::strchr(line, ')');
        const char* tag_start = close != nullptr ? close + 1 : line + 1;
        while (*tag_start == ' ') ++tag_start;

        const char* colon = std::strchr(tag_start, ':');
        if (colon != nullptr && colon > tag_start) {
            const size_t tag_len = std::min<size_t>(static_cast<size_t>(colon - tag_start), sizeof(entry.tag) - 1);
            std::memcpy(entry.tag, tag_start, tag_len);
            entry.tag[tag_len] = '\0';
            message = colon + 1;
            while (*message == ' ') ++message;
        }
    }

    CopyText(entry.message, sizeof(entry.message), message);

    // Non-blocking: logging tidak boleh menahan task audio/network.
    xQueueSend(queue_, &entry, 0);
}

bool RemoteDeviceLog::BuildBatchJson(
    const std::string& device_id,
    std::string& json_body,
    size_t max_entries
)
{
    if (queue_ == nullptr || uxQueueMessagesWaiting(queue_) == 0) return false;

    cJSON* root = cJSON_CreateObject();
    cJSON* logs = cJSON_CreateArray();
    if (root == nullptr || logs == nullptr) {
        if (root != nullptr) cJSON_Delete(root);
        if (logs != nullptr) cJSON_Delete(logs);
        return false;
    }

    cJSON_AddStringToObject(root, "device_id", device_id.c_str());
    cJSON_AddItemToObject(root, "logs", logs);

    Entry entry{};
    size_t count = 0;
    while (count < max_entries && xQueueReceive(queue_, &entry, 0) == pdTRUE) {
        cJSON* item = cJSON_CreateObject();
        if (item == nullptr) break;

        cJSON_AddNumberToObject(item, "uptime_ms", entry.uptime_ms);
        cJSON_AddStringToObject(item, "level", entry.level);
        cJSON_AddStringToObject(item, "tag", entry.tag);
        cJSON_AddStringToObject(item, "message", entry.message);
        cJSON_AddItemToArray(logs, item);
        ++count;
    }

    if (count == 0) {
        cJSON_Delete(root);
        return false;
    }

    char* encoded = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (encoded == nullptr) return false;

    json_body.assign(encoded);
    cJSON_free(encoded);
    return true;
}
