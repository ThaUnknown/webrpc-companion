#pragma once

#include <string>
#include <string_view>

#include "discord_ipc.hpp"
#include "game_detection.hpp"
#include "native_messaging.hpp"

class Host : public IpcCallbacks {
public:
    Host();
    int run();

    void onIpcConnect(uint64_t id, const std::string& clientId, int version) override;
    void onIpcRequest(uint64_t id, const std::string& payload) override;
    void onIpcDisconnect(uint64_t id) override;

private:
    void handleMessage(std::string_view message);
    void onReady();
    void onGame(const DetectedGame& game, bool running);
    void send(const std::string& json);
    void sendCommand(const char* message);

    NativeMessaging native;
    DiscordIpc ipc;
    GameDetection detection;

    bool rendererReady = false;
    bool ipcStarted = false;
};
