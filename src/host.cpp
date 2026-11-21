#include "host.hpp"

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "json.hpp"
#include "log.hpp"
#include "platform.hpp"
#include "protocol.hpp"

namespace {

std::optional<uint64_t> parseId(std::string_view text) {
    uint64_t value;
    const char* end = text.data() + text.size();
    auto [last, error] = std::from_chars(text.data(), end, value);
    if (error != std::errc() || last != end) return std::nullopt;

    return value;
}

}  // namespace

Host::Host() : ipc(*this), detection([this](const DetectedGame& game, bool running) { onGame(game, running); }) {}

int Host::run() {
    setLogFile(dataDir() / "host.log");

    {
        JsonWriter json;
        json.string("message", "hello");
        json.beginObject("data");
        json.number("protocol", PROTOCOL_VERSION);
        json.string("version", VENCORD_RPC_VERSION);
        json.string("installPath", pathToUtf8(executablePath()));
        json.endObject();
        send(json.finish());
    }

    std::string message;
    while (native.read(message)) handleMessage(message);

    detection.stop();
    ipc.stop();
    return 0;
}

void Host::handleMessage(std::string_view message) {
    const JsonDoc doc = parseJson(message);
    if (!doc) {
        logMessage("Failed to parse a native message");
        return;
    }

    yyjson_val* root = yyjson_doc_get_root(doc.get());
    const std::string_view command = jsonString(root, "message");
    yyjson_val* data = yyjson_obj_get(root, "data");

    if (command == "ready") {
        onReady();
    } else if (command == "detectable-db") {
        const std::string_view database = jsonString(data, "json");
        if (database.empty()) {
            logMessage("The extension sent no detectable database: %s", std::string(jsonString(data, "error")).c_str());
            return;
        }

        if (detection.setDatabase(database) && rendererReady) detection.start();
    } else if (command == "rpc:send") {
        const auto id = parseId(jsonString(data, "id"));
        const std::string_view payload = jsonString(data, "json");
        if (id && !payload.empty()) ipc.send(*id, std::string(payload));
    } else if (command == "rpc:close") {
        const auto id = parseId(jsonString(data, "id"));
        const int code = (int)jsonInt(data, "code", CLOSE_NORMAL);
        if (id) ipc.close(*id, code, std::string(jsonString(data, "message")));
    } else if (command == "stop") {
        rendererReady = false;
        detection.stop();
        ipc.stop();
        ipcStarted = false;
        sendCommand("rpc:reset");
    } else if (command == "ping") {
        sendCommand("pong");
    }
}

void Host::onReady() {
    rendererReady = true;

    if (!ipcStarted) ipcStarted = ipc.start();
    else ipc.closeAll(CLOSE_NORMAL, "Renderer reloaded");

    sendCommand("rpc:reset");
    detection.start();
}

void Host::onGame(const DetectedGame& game, bool running) {
    JsonWriter json;
    json.string("message", "rpc:game");
    json.beginObject("data");
    json.beginObject("game");
    json.string("id", game.id);
    json.string("name", game.name);
    json.number("pid", game.pid);
    json.string("path", game.path);
    json.endObject();
    json.boolean("running", running);
    json.endObject();
    send(json.finish());
}

void Host::onIpcConnect(uint64_t id, const std::string& clientId, int version) {
    JsonWriter json;
    json.string("message", "rpc:connect");
    json.beginObject("data");
    json.string("id", std::to_string(id));
    json.null("origin");
    json.string("transport", "ipc");

    if (clientId.empty()) json.null("clientId");
    else json.string("clientId", clientId);

    json.number("version", version);
    json.endObject();
    send(json.finish());
}

void Host::onIpcRequest(uint64_t id, const std::string& payload) {
    JsonWriter json;
    json.string("message", "rpc:request");
    json.beginObject("data");
    json.string("id", std::to_string(id));
    json.string("payload", payload);
    json.endObject();
    send(json.finish());
}

void Host::onIpcDisconnect(uint64_t id) {
    JsonWriter json;
    json.string("message", "rpc:disconnect");
    json.string("data", std::to_string(id));
    send(json.finish());
}

void Host::send(const std::string& json) {
    native.write(json);
}

void Host::sendCommand(const char* message) {
    JsonWriter json;
    json.string("message", message);
    send(json.finish());
}
