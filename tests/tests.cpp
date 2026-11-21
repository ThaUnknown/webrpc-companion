#include <cstdio>
#include <string>

#include "json.hpp"

namespace {

int failures = 0;

void check(bool condition, const char* name) {
    if (condition) return;

    fprintf(stderr, "FAIL: %s\n", name);
    failures++;
}

}  // namespace

int main() {
    check(jsonEscape("a\"b\\c\n\t") == "a\\\"b\\\\c\\n\\t", "json escape");

    JsonWriter hello;
    hello.string("message", "hello");
    hello.beginObject("data");
    hello.number("protocol", 2);
    hello.boolean("ok", true);
    hello.null("clientId");
    hello.number("version", 1);
    hello.endObject();

    check(
        hello.finish() == "{\"message\":\"hello\",\"data\":{\"protocol\":2,\"ok\":true,\"clientId\":null,\"version\":1}}",
        "json writer"
    );

    JsonWriter rpc;
    rpc.string("message", "rpc:connect");
    rpc.beginObject("data");
    rpc.string("id", "1");
    rpc.null("origin");
    rpc.string("transport", "ipc");
    rpc.null("clientId");
    rpc.number("version", 1);
    rpc.endObject();

    check(
        rpc.finish() == "{\"message\":\"rpc:connect\",\"data\":{\"id\":\"1\",\"origin\":null,\"transport\":\"ipc\",\"clientId\":null,\"version\":1}}",
        "rpc envelope"
    );

    JsonWriter disconnect;
    disconnect.string("message", "rpc:disconnect");
    disconnect.string("data", "1");
    check(disconnect.finish() == "{\"message\":\"rpc:disconnect\",\"data\":\"1\"}", "disconnect envelope");

    JsonWriter reset;
    reset.string("message", "rpc:reset");
    check(reset.finish() == "{\"message\":\"rpc:reset\"}", "reset envelope");

    std::string message = "{\"message\":\"detectable-db\",\"data\":{\"json\":null,\"error\":\"missing\"}}";
    const JsonDoc doc = parseJson(message);
    check(doc != nullptr, "parse json");

    yyjson_val* root = yyjson_doc_get_root(doc.get());
    yyjson_val* data = yyjson_obj_get(root, "data");
    check(jsonString(root, "message") == "detectable-db", "read string");
    check(jsonString(data, "error") == "missing", "read data");

    return failures == 0 ? 0 : 1;
}
