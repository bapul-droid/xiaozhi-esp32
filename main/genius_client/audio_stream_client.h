#pragma once

#include <atomic>
#include <string>


class AudioStreamClient {
public:
    AudioStreamClient() = default;

    bool Download(
        const std::string& url,
        const std::atomic<bool>& stop_requested
    );
};
