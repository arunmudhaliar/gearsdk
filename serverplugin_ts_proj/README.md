## 🖥️ Platform Supported

*OS:* **macOS** (Intel)

*Node.js Version:*
Requires Node.js v16.20.2+ and N-API compatibility

*Architecture:*
Supports x86_64

Ensure you have the necessary dependencies installed before using the plugin. 

**Note: This plugin is under development phase. Will notify once its release ready.**

---

# gsdk server plugin (TypeScript)

This TypeScript module provides a **server plugin** that integrates with a QUIC-based networking framework. It dynamically loads a native `.node` library (`libserverplugin`), selecting either the **debug** or **release** version based on availability. The module supports both **stateful** and **stateless** server implementations.

---

## 🚀 Stateless Server (`qh3server`)

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

### **Ideal Use Case**

✅ High-throughput microservices handling QUIC requests.
✅ Routing and proxying requests where session persistence is not needed.

---

## 🎮 Stateful Server (`qserver`)

A **stateful server** maintains session data for clients, making it suitable for applications like real-time multiplayer games or chat systems.

### **Key Features**

- Uses `spawn_qserver` to initialize a stateful server.
- Manages **rooms** (persistent game/match sessions) via:
  - `room_event_create`: Handles room creation.
  - `room_event_player_added`: Manages players joining rooms.
  - `room_event_message`: Facilitates real-time communication.
- Supports **broadcasting** messages:
  - `room_broadcast`: Sends messages to all clients.
  - `room_broadcast_except`: Sends messages to all except a specific client.
  - `room_send_to`: Sends messages to a specific client.

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

🔥 Get started by selecting the server type that fits your use case! 🚀
