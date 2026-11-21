# WebRPC native protocol

The extension starts the companion with `chrome.runtime.connectNative("dev.vencord.rpc")`.

## Transport

Native messaging frames:

- 4-byte little-endian length.
- UTF-8 JSON payload.

Every message uses the same envelope:

```json
{ "message": "rpc:connect", "data": { } }
```

`message` selects the command. `data` is optional and holds the command arguments.

The host writes logs to stderr and to `host.log`. It never writes logs to stdout.

## Messages from the companion

| Message | Data | Meaning |
| --- | --- | --- |
| `hello` | `protocol`, `version`, `installPath` | Sent when the host starts. |
| `rpc:connect` | `id`, `clientId`, `version`, `origin`, `transport` | An RPC client connected. |
| `rpc:request` | `id`, `payload` | The client sent a command. `payload` is a JSON string. |
| `rpc:disconnect` | `id` (string) | The client disconnected. |
| `rpc:game` | `game`, `running` | A game started or stopped. `game` has `id`, `name`, `pid`, and `path`. |
| `rpc:reset` | | Close all RPC clients and clear detected games. |
| `pong` | | Answer to `ping`. |

## Messages from the extension

| Message | Data | Meaning |
| --- | --- | --- |
| `ready` | | The renderer registered its RPC transport. The host starts or restarts the RPC server and game detection. |
| `detectable-db` | `json`, `error` | Discord's detectable games database. `json` is a JSON string. |
| `rpc:send` | `id`, `json` | Send a frame to an RPC client. |
| `rpc:close` | `id`, `code`, `message` | Close an RPC client. |
| `stop` | | The renderer stopped. Close clients and stop game detection. |
| `ping` | | Keepalive. |

## Lifecycle

1. The host starts and sends `hello` with `protocol: 2`.
2. The extension sends `ready`.
3. The host starts the Discord IPC server.
   - Windows: `\\.\pipe\discord-ipc-0` to `-9`.
   - Linux and macOS: `$XDG_RUNTIME_DIR` or the temporary directory, `discord-ipc-0` to `-9`.
4. The extension sends `detectable-db`.
5. The host scans every 5 seconds and sends `rpc:game` messages. Discord decides whether to show the activity.
6. The host exits when stdin closes.

## Discord IPC frames

- 8-byte header: opcode (int32 LE), payload size (int32 LE).
- Opcodes: `0` handshake, `1` frame, `2` close, `3` ping, `4` pong.
- The maximum frame size is 3 MiB.
- The handshake is `{"v":1,"client_id":"..."}`.

## Install behavior

The host installs itself when it starts without a native messaging origin argument.

- The installer is silent.
- It never opens a window.
- It writes a manifest with the published extension ID and the development extension ID.
