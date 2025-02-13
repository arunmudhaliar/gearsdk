## 📌 Features

- Supports **qh3router**, **qh3server (stateless)**, and **game server (stateful)** operations.
- Provides callback interfaces for handling server events.

## 🖥️ Platform Supported

*OS:* **macOS** (x86_64), Linux (x86_64)

*Node.js Version:*
Requires Node.js v16.20.2+ and N-API compatibility

Ensure you have the necessary dependencies installed before using the plugin. 

**Note: This plugin is under development phase. Will notify once its release ready.**

### 🚀 Installation

```sh
# todo
```

---

# **gsdk server plugin (TypeScript)**

This TypeScript module provides a **server plugin** that integrates with a QUIC-based networking framework. It dynamically loads a native `.node` library (`libserverplugin`), selecting either the **debug** or **release** version based on availability. The module supports both **stateful** and **stateless** server implementations.

---

# qh3router

## Overview
`spawn_qh3router` initializes and starts a router with the given parameters. It sets up the necessary connections to MongoDB, Redis, and ZooKeeper, while providing callback hooks for various lifecycle events.

## Usage

### Function Signature
```typescript
spawn_qh3router(
    routerAddress: string,
    mongodbUri: string,
    redisAddress: string,
    zkUri: string,
    rootDir: string,
    inf_file: string,
    commandPort: number,
    routerPortReturn: number,
    appId: string,
    preStartCallback: type_on_router_pre_start,
    startCallback: type_on_router_start,
    stopCallback: type_on_router_stop,
    errorCallback: type_on_router_error,
): void;
```

### Parameters
- **routerAddress** *(string)* - The address where the router will listen.
- **mongodbUri** *(string)* - The MongoDB connection URI.
- **redisAddress** *(string)* - The Redis server address.
- **zkUri** *(string)* - The ZooKeeper connection URI.
- **rootDir** *(string)* - The root directory for storing runtime files.
- **inf_file** *(string)* - Path to the interface file.
- **commandPort** *(number)* - The port used for command communication.
- **routerPortReturn** *(number)* - The router return port.
- **appId** *(string)* - The application ID associated with this router.
- **preStartCallback** *(type_on_router_pre_start)* - Callback function executed before the router starts.
- **startCallback** *(type_on_router_start)* - Callback function executed when the router starts successfully.
- **stopCallback** *(type_on_router_stop)* - Callback function executed when the router stops.
- **errorCallback** *(type_on_router_error)* - Callback function executed when an error occurs.

## Example

```typescript
spawn_qh3router(
    "0.0.0.0:5000",                 // Router Address
    "mongodb://localhost:27017",    // MongoDB URI
    "redis://localhost:6379",       // Redis Address
    "zk://localhost:2181",          // ZooKeeper URI
    "/var/qh3router",               // Root Directory
    "/etc/qh3router/config.inf",    // Interface File
    8081,                            // Command Port
    6000,                            // Router Port Return
    "qh3router-app",                // Application ID
    (router: any, cd_data: any) => console.log("Pre-start hook"), // Pre-start callback
    (router: any) => console.log("Router started"), // Start callback
    (router: any) => console.log("Router stopped"), // Stop callback
    (router: any, error_code: number) => console.error("Error: ", error_code) // Error callback
);
```

## Notes
- Ensure that MongoDB, Redis, and ZooKeeper are running and accessible before calling `spawn_qh3router`.
- Callbacks should be defined to handle lifecycle events properly.
- The router will bind to the specified `routerAddress` and listen for incoming connections.

---


# 🚀 Stateless Server (`type qh3server`)

A **stateless server** handles each request independently, without storing session-specific information between client interactions.

### **Key Features**

- Uses `spawn_qh3server` to initialize a QUIC server.
- Supports **event-driven request handling** using callback functions:
  - `type_on_server_parse`: Handles incoming requests.
  - `type_on_server_error`: Handles errors.
- Uses `qh3server_try_send_response` to send responses without maintaining session data.
- Logging and statistics tracking via:
  - `qh3server_logfile`
  - `qh3server_stats_count`

## Usage

The `spawn_qh3server` function initializes and starts a `qh3server` instance with the specified configuration parameters.

### Function Signature
```typescript
spawn_qh3server(
    native_router: any,
    server_address: string,
    mongodb_uri: string,
    redis_address: string,
    zk_uri: string,
    root_dir: string,
    inf_file: string,
    command_port: number,
    router_port_return: number,
    app_id: string,
    pre_start_cb: type_on_qh3server_pre_start,
    start_cb: type_on_qh3server_start,
    stop_cb: type_on_qh3server_stop,
    error_cb: type_on_qh3server_error,
    parse_cb: type_on_qh3server_parse
): void;
```

### Parameters

| Parameter           | Type                         | Description |
|---------------------|----------------------------|-------------|
| `native_router`    | `any`                        | The native router instance for handling requests. |
| `server_address`   | `string`                     | The address where the server will be hosted (e.g., `0.0.0.0:443`). |
| `mongodb_uri`      | `string`                     | The MongoDB connection URI. |
| `redis_address`    | `string`                     | The Redis server address (e.g., `redis://localhost:6379`). |
| `zk_uri`           | `string`                     | The ZooKeeper connection URI. |
| `root_dir`         | `string`                     | The root directory for server files. |
| `inf_file`         | `string`                     | Path to the interface configuration file. |
| `command_port`     | `number`                     | The port for command communication. |
| `router_port_return` | `number`                   | The port used for router responses. |
| `app_id`           | `string`                     | Application identifier for server instance. |
| `pre_start_cb`     | `type_on_qh3server_pre_start` | Callback triggered before the server starts. |
| `start_cb`         | `type_on_qh3server_start`     | Callback triggered when the server starts. |
| `stop_cb`          | `type_on_qh3server_stop`      | Callback triggered when the server stops. |
| `error_cb`         | `type_on_qh3server_error`    | Callback triggered when an error occurs. |
| `parse_cb`         | `type_on_qh3server_parse`    | Callback for parsing incoming data. |

### Example Usage
```typescript
spawn_qh3server(
    nativeRouterInstance,
    "0.0.0.0:443",
    "mongodb://localhost:27017/qh3db",
    "redis://localhost:6379",
    "zk://localhost:2181",
    "/var/qh3server",
    "/etc/qh3server/config.inf",
    8080,
    9000,
    "qh3-app",
    (native_server: serversdk.qh3server_ptr) => console.log("Server is preparing to start..."),
    (native_server: serversdk.qh3server_ptr, ip: string, port: number) => console.log("Server started successfully."),
    (native_server: serversdk.qh3server_ptr) => console.log("Server stopped."),
    (native_server: serversdk.qh3server_ptr, error_code: number) => console.error("Server encountered an error:", error_code),
    (native_server: serversdk.qh3server_ptr, cid: any, cid_len: number, path: string, buffer: string, headers: string) => console.log("Parsing request data:", buffer)
);
```

### **Ideal Use Case**

✅ High-throughput microservices handling QUIC requests.
✅ Routing and proxying requests where session persistence is not needed.

---

# 🎮 Stateful Game Server (`type qserver`)

A **stateful server** maintains session data for clients, making it suitable for applications like real-time multiplayer games or chat systems.

### **Key Features**

- Uses `spawn_game_server` to initialize a stateful game server.
- Manages **rooms** (persistent game/match sessions) via:
  - `room_event_create`: Handles room creation.
  - `type_on_room_event_start`: Room start event.
  - `type_on_room_event_stop`: Room stop event.
  - `room_event_player_added`: Manages players joining rooms.
  - `room_event_player_removed`: Manages players removed from rooms.
  - `type_on_room_event_countdown_to_start`: Room count down event.
  - `type_on_room_event_countdown_cancelled`: Room count down cancel event.
  - `room_event_message`: Facilitates real-time communication.
- Supports **broadcasting** messages:
  - `room_broadcast`: Sends messages to all clients.
  - `room_broadcast_except`: Sends messages to all except a specific client.
  - `room_send_to`: Sends messages to a specific client.

## 📖 Usage

```typescript
import { serversdk } from './serversdk';

const config: serversdk.qserver_input_config = {
    server_address: "127.0.0.1",
    redis_address: "localhost:6379",
    redis_user: "",
    redis_password: "",
    zk_uri: "zk://localhost:2181",
    root_dir: "/data",
    inf_file: "server.inf",
    app_id: "app-1234"
};

serversdk.sdklib.spawn_game_server(
    config.server_address,
    config.redis_address,
    config.zk_uri,
    config.root_dir,
    config.inf_file,
    config.app_id,
    (native_server: serversdk.qserver_ptr) => console.log("Game server pre-start"),
    (native_server: serversdk.qserver_ptr, ip: string, port: number) => console.log("Game server started"),
    (native_server: serversdk.qserver_ptr) => console.log("Game server stopped"),
    (native_server: serversdk.qserver_ptr, error_code: number) => console.error("Game server error", err),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => console.log("Room created", room),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => console.log("Room start", room),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, pid: string, cid_hash: number) => console.log("player added", room),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, pid: string, cid_hash: number, message: string) => console.log("room message", room),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, pid: string, cid_hash: number) => console.log("player removed", room),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => console.log("Room end", room),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, count: number, max_count: number) => console.log("count down", room),
    (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => console.log("count down calncelled", room)
);
```

### Server Event Callbacks

- **Router Events**: `type_on_router_pre_start`, `type_on_router_start`, `type_on_router_stop`, `type_on_router_error`
- **qh3server Events**: `type_on_qh3server_pre_start`, `type_on_qh3server_start`, `type_on_qh3server_stop`, `type_on_qh3server_error`, `type_on_qh3server_parse`
- **Game Server Events**: `type_on_qserver_pre_start`, `type_on_qserver_start`, `type_on_qserver_stop`, `type_on_qserver_error`
- **Room Events**: `type_on_room_event_create`, `type_on_room_event_start`, `type_on_room_event_player_added`, `type_on_room_event_message`, `type_on_room_event_player_removed`, `type_on_room_event_end`, `type_on_room_event_countdown_to_start`, `type_on_room_event_countdown_cancelled`

### **Ideal Use Case**

✅ Multiplayer game servers (e.g., battle royale, co-op games).
✅ Real-time chat or collaboration applications.

---

## 📊 Comparison: Stateful vs. Stateless

| Feature                  | Stateless (`qh3server`) | Stateful (`qserver`)    |
| ------------------------ | ----------------------- | ----------------------- |
| Maintains Sessions       | ❌ No                    | ✅ Yes                   |
| Handles Real-Time Events | ❌ No                    | ✅ Yes                   |
| Broadcast Messages       | ❌ No                    | ✅ Yes                   |
| Use Case                 | API servers, proxies    | Multiplayer games, chat |

---

## 📌 Conclusion

- **Use `qh3server` (stateless)** for request-response APIs.
- **Use `qserver` (stateful)** for applications that need persistent sessions.

## 📝 Notes

- Ensure that the necessary dependencies (e.g., Redis, MongoDB, Zookeeper) are correctly set up before running the servers.

🔥 Get started by selecting the server type that fits your use case! 🚀
