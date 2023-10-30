
#ifndef QUICHE_H
#define QUICHE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#endif

#ifdef __unix__
#include <sys/types.h>
#endif
#ifdef _MSC_VER
#include <BaseTsd.h>
#define ssize_t SSIZE_T
#endif

#define QUICHE_H3_APPLICATION_PROTOCOL "\x02h3"

enum Event_type {
    /// Request/response headers were received.
    Headers,

    /// Data was received.
    ///
    /// This indicates that the application can use the [`recv_body()`] method
    /// to retrieve the data from the stream.
    ///
    /// Note that [`recv_body()`] will need to be called repeatedly until the
    /// [`Done`] value is returned, as the event will not be re-armed until all
    /// buffered data is read.
    ///
    /// [`recv_body()`]: struct.Connection.html#method.recv_body
    /// [`Done`]: enum.Error.html#variant.Done
    Data,

    /// Stream was closed,
    Finished,

    /// Stream was reset.
    ///
    /// The associated data represents the error code sent by the peer.
    Reset,

    /// PRIORITY_UPDATE was received.
    ///
    /// This indicates that the application can use the
    /// [`take_last_priority_update()`] method to take the last received
    /// PRIORITY_UPDATE for a specified stream.
    ///
    /// This event is triggered once per stream until the last PRIORITY_UPDATE
    /// is taken. It is recommended that applications defer taking the
    /// PRIORITY_UPDATE until after [`poll()`] returns [`Done`].
    ///
    /// [`take_last_priority_update()`]: struct.Connection.html#method.take_last_priority_update
    /// [`poll()`]: struct.Connection.html#method.poll
    /// [`Done`]: enum.Error.html#variant.Done
    PriorityUpdate,

    /// GOAWAY was received.
    GoAway
};

enum quiche_error {
    // There is no more work to do.
    QUICHE_ERR_DONE = -1,

    // The provided buffer is too short.
    QUICHE_ERR_BUFFER_TOO_SHORT = -2,

    // The provided packet cannot be parsed because its version is unknown.
    QUICHE_ERR_UNKNOWN_VERSION = -3,

    // The provided packet cannot be parsed because it contains an invalid
    // frame.
    QUICHE_ERR_INVALID_FRAME = -4,

    // The provided packet cannot be parsed.
    QUICHE_ERR_INVALID_PACKET = -5,

    // The operation cannot be completed because the connection is in an
    // invalid state.
    QUICHE_ERR_INVALID_STATE = -6,

    // The operation cannot be completed because the stream is in an
    // invalid state.
    QUICHE_ERR_INVALID_STREAM_STATE = -7,

    // The peer's transport params cannot be parsed.
    QUICHE_ERR_INVALID_TRANSPORT_PARAM = -8,

    // A cryptographic operation failed.
    QUICHE_ERR_CRYPTO_FAIL = -9,

    // The TLS handshake failed.
    QUICHE_ERR_TLS_FAIL = -10,

    // The peer violated the local flow control limits.
    QUICHE_ERR_FLOW_CONTROL = -11,

    // The peer violated the local stream limits.
    QUICHE_ERR_STREAM_LIMIT = -12,

    // The specified stream was stopped by the peer.
    QUICHE_ERR_STREAM_STOPPED = -15,

    // The specified stream was reset by the peer.
    QUICHE_ERR_STREAM_RESET = -16,

    // The received data exceeds the stream's final size.
    QUICHE_ERR_FINAL_SIZE = -13,

    // Error in congestion control.
    QUICHE_ERR_CONGESTION_CONTROL = -14,

    // Too many identifiers were provided.
    QUICHE_ERR_ID_LIMIT = -17,

    // Not enough available identifiers.
    QUICHE_ERR_OUT_OF_IDENTIFIERS = -18,

    // Error in key update.
    QUICHE_ERR_KEY_UPDATE = -19,
};
/**
 * The current QUIC wire version.
 */
#define PROTOCOL_VERSION 0x00000001 //PROTOCOL_VERSION_V1

/**
 * The minimum length of Initial packets sent by a client.
 */
#define MIN_CLIENT_INITIAL_LEN 1200

#define MAX_CRYPTO_OVERHEAD 8

#define MAX_DGRAM_OVERHEAD 2

#define MAX_STREAM_OVERHEAD 12

#define MAX_STREAM_SIZE (1 << 62)

#define DATA_FRAME_TYPE_ID 0

#define HEADERS_FRAME_TYPE_ID 1

#define CANCEL_PUSH_FRAME_TYPE_ID 3

#define SETTINGS_FRAME_TYPE_ID 4

#define PUSH_PROMISE_FRAME_TYPE_ID 5

#define GOAWAY_FRAME_TYPE_ID 7

#define MAX_PUSH_FRAME_TYPE_ID 13

#define PRIORITY_UPDATE_FRAME_REQUEST_TYPE_ID 984832

#define PRIORITY_UPDATE_FRAME_PUSH_TYPE_ID 984833

#define SETTINGS_QPACK_MAX_TABLE_CAPACITY 1

#define SETTINGS_MAX_FIELD_SECTION_SIZE 6

#define SETTINGS_QPACK_BLOCKED_STREAMS 7

#define SETTINGS_ENABLE_CONNECT_PROTOCOL 8

#define SETTINGS_H3_DATAGRAM_00 630

#define SETTINGS_H3_DATAGRAM 51

#define HTTP3_CONTROL_STREAM_TYPE_ID 0

#define HTTP3_PUSH_STREAM_TYPE_ID 1

#define QPACK_ENCODER_STREAM_TYPE_ID 2

#define QPACK_DECODER_STREAM_TYPE_ID 3

#define MAX_CID_LEN 20

#define MAX_PKT_NUM_LEN 4

#define N_RTT_SAMPLE 8

#define CSS_GROWTH_DIVISOR 4

#define CSS_ROUNDS 5

/**
 * The maximum size of the receiver stream flow control window.
 */
#define MAX_STREAM_WINDOW ((16 * 1024) * 1024)

/**
 * Available congestion control algorithms.
 *
 * This enum provides currently available list of congestion control
 * algorithms.
 */
typedef enum CongestionControlAlgorithm {
  /**
   * Reno congestion control algorithm. `reno` in a string form.
   */
  Reno = 0,
  /**
   * CUBIC congestion control algorithm (default). `cubic` in a string form.
   */
  CUBIC = 1,
  /**
   * BBR congestion control algorithm. `bbr` in a string form.
   */
  BBR = 2,
  /**
   * BBRv2 congestion control algorithm. `bbr2` in a string form.
   */
  BBR2 = 3,
} CongestionControlAlgorithm;

/**
 * The side of the stream to be shut down.
 *
 * This should be used when calling [`stream_shutdown()`].
 *
 * [`stream_shutdown()`]: struct.Connection.html#method.stream_shutdown
 */
typedef enum Shutdown {
  /**
   * Stop receiving stream data.
   */
  Read = 0,
  /**
   * Stop sending stream data.
   */
  Write = 1,
} Shutdown;

/**
 * Stores configuration shared between multiple connections.
 */
typedef struct Config Config;

/**
 * A QUIC connection.
 */
typedef struct Connection Connection;

/**
 * An HTTP/3 connection event.
 */
typedef struct Event Event;

/**
 * An iterator over QUIC streams.
 */
typedef struct StreamIter StreamIter;

/**
 * QUIC Transport Parameters
 */
typedef struct TransportParams TransportParams;

typedef struct RecvInfo {
    const struct sockaddr *from;
  socklen_t from_len;
    const struct sockaddr *to;
  socklen_t to_len;
} RecvInfo;

typedef struct SendInfo {
    struct sockaddr_storage from;
  socklen_t from_len;
    struct sockaddr_storage to;
  socklen_t to_len;
    struct timespec at;
} SendInfo;

typedef struct PathStats {
    struct sockaddr_storage local_addr;
  socklen_t local_addr_len;
    struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len;
  ssize_t validation_state;
  bool active;
  uintptr_t recv;
  uintptr_t sent;
  uintptr_t lost;
  uintptr_t retrans;
  uint64_t rtt;
  uintptr_t cwnd;
  uint64_t sent_bytes;
  uint64_t recv_bytes;
  uint64_t lost_bytes;
  uint64_t stream_retrans_bytes;
  uintptr_t pmtu;
  uint64_t delivery_rate;
} PathStats;

typedef struct Stats {
  uintptr_t recv;
  uintptr_t sent;
  uintptr_t lost;
  uintptr_t retrans;
  uint64_t sent_bytes;
  uint64_t recv_bytes;
  uint64_t lost_bytes;
  uint64_t stream_retrans_bytes;
  uintptr_t paths_count;
  struct PathStats paths[8];
} Stats;

typedef struct Header {
  uint8_t *name;
  uintptr_t name_len;
  uint8_t *value;
  uintptr_t value_len;
} Header;

/**
 * Extensible Priorities parameters.
 *
 * The `TryFrom` trait supports constructing this object from the serialized
 * Structured Fields Dictionary field value. I.e, use `TryFrom` to parse the
 * value of a Priority header field or a PRIORITY_UPDATE frame. Using this
 * trait requires the `sfv` feature to be enabled.
 */
typedef struct Priority {
  uint8_t urgency;
  bool incremental;
} Priority;

const uint8_t *quiche_version(void);

int quiche_enable_debug_logging(void (*cb)(const uint8_t *line, void *argp), void *argp);

struct Config *quiche_config_new(uint32_t version);

int quiche_config_load_cert_chain_from_pem_file(struct Config *config, const char *path);

int quiche_config_load_priv_key_from_pem_file(struct Config *config, const char *path);

int quiche_config_load_verify_locations_from_file(struct Config *config, const char *path);

int quiche_config_load_verify_locations_from_directory(struct Config *config, const char *path);

void quiche_config_verify_peer(struct Config *config, bool v);

void quiche_config_grease(struct Config *config, bool v);

void quiche_config_log_keys(struct Config *config);

void quiche_config_enable_early_data(struct Config *config);

/**
 * Corresponds to the `Config::set_application_protos_wire_format` Rust
 * function.
 */
int quiche_config_set_application_protos(struct Config *config,
                                         const uint8_t *protos,
                                         size_t protos_len);

void quiche_config_set_max_idle_timeout(struct Config *config, uint64_t v);

void quiche_config_set_max_recv_udp_payload_size(struct Config *config, size_t v);

void quiche_config_set_initial_max_data(struct Config *config, uint64_t v);

void quiche_config_set_initial_max_stream_data_bidi_local(struct Config *config, uint64_t v);

void quiche_config_set_initial_max_stream_data_bidi_remote(struct Config *config, uint64_t v);

void quiche_config_set_initial_max_stream_data_uni(struct Config *config, uint64_t v);

void quiche_config_set_initial_max_streams_bidi(struct Config *config, uint64_t v);

void quiche_config_set_initial_max_streams_uni(struct Config *config, uint64_t v);

void quiche_config_set_ack_delay_exponent(struct Config *config, uint64_t v);

void quiche_config_set_max_ack_delay(struct Config *config, uint64_t v);

void quiche_config_set_disable_active_migration(struct Config *config, bool v);

int quiche_config_set_cc_algorithm_name(struct Config *config, const char *name);

void quiche_config_set_cc_algorithm(struct Config *config, enum CongestionControlAlgorithm algo);

void quiche_config_set_initial_congestion_window_packets(struct Config *config, size_t packets);

void quiche_config_enable_hystart(struct Config *config, bool v);

void quiche_config_enable_pacing(struct Config *config, bool v);

void quiche_config_set_max_pacing_rate(struct Config *config, uint64_t v);

void quiche_config_enable_dgram(struct Config *config,
                                bool enabled,
                                size_t recv_queue_len,
                                size_t send_queue_len);

void quiche_config_set_max_send_udp_payload_size(struct Config *config, size_t v);

void quiche_config_set_max_connection_window(struct Config *config, uint64_t v);

void quiche_config_set_max_stream_window(struct Config *config, uint64_t v);

void quiche_config_set_active_connection_id_limit(struct Config *config, uint64_t v);

void quiche_config_set_stateless_reset_token(struct Config *config, const uint8_t *v);

void quiche_config_free(struct Config *config);

int quiche_header_info(uint8_t *buf,
                       size_t buf_len,
                       size_t dcil,
                       uint32_t *version,
                       uint8_t *ty,
                       uint8_t *scid,
                       size_t *scid_len,
                       uint8_t *dcid,
                       size_t *dcid_len,
                       uint8_t *token,
                       size_t *token_len);

struct Connection *quiche_accept(const uint8_t *scid,
                                 size_t scid_len,
                                 const uint8_t *odcid,
                                 size_t odcid_len,
                                 const struct sockaddr *local,
                                 socklen_t local_len,
                                 const struct sockaddr *peer,
                                 socklen_t peer_len,
                                 struct Config *config);

struct Connection *quiche_connect(const char *server_name,
                                  const uint8_t *scid,
                                  size_t scid_len,
                                  const struct sockaddr *local,
                                  socklen_t local_len,
                                  const struct sockaddr *peer,
                                  socklen_t peer_len,
                                  struct Config *config);

ssize_t quiche_negotiate_version(const uint8_t *scid,
                                 size_t scid_len,
                                 const uint8_t *dcid,
                                 size_t dcid_len,
                                 uint8_t *out,
                                 size_t out_len);

bool quiche_version_is_supported(uint32_t version);

ssize_t quiche_retry(const uint8_t *scid,
                     size_t scid_len,
                     const uint8_t *dcid,
                     size_t dcid_len,
                     const uint8_t *new_scid,
                     size_t new_scid_len,
                     const uint8_t *token,
                     size_t token_len,
                     uint32_t version,
                     uint8_t *out,
                     size_t out_len);

struct Connection *quiche_conn_new_with_tls(const uint8_t *scid,
                                            size_t scid_len,
                                            const uint8_t *odcid,
                                            size_t odcid_len,
                                            const struct sockaddr *local,
                                            socklen_t local_len,
                                            const struct sockaddr *peer,
                                            socklen_t peer_len,
                                            const struct Config *config,
                                            void *ssl,
                                            bool is_server);

bool quiche_conn_set_keylog_path(struct Connection *conn, const char *path);

void quiche_conn_set_keylog_fd(struct Connection *conn, int fd);

bool quiche_conn_set_qlog_path(struct Connection *conn,
                               const char *path,
                               const char *log_title,
                               const char *log_desc);

void quiche_conn_set_qlog_fd(struct Connection *conn,
                             int fd,
                             const char *log_title,
                             const char *log_desc);

int quiche_conn_set_session(struct Connection *conn, const uint8_t *buf, size_t buf_len);

ssize_t quiche_conn_recv(struct Connection *conn,
                         uint8_t *buf,
                         size_t buf_len,
                         const struct RecvInfo *info);

ssize_t quiche_conn_send(struct Connection *conn,
                         uint8_t *out,
                         size_t out_len,
                         struct SendInfo *out_info);

ssize_t quiche_conn_stream_recv(struct Connection *conn,
                                uint64_t stream_id,
                                uint8_t *out,
                                size_t out_len,
                                bool *fin);

ssize_t quiche_conn_stream_send(struct Connection *conn,
                                uint64_t stream_id,
                                const uint8_t *buf,
                                size_t buf_len,
                                bool fin);

int quiche_conn_stream_priority(struct Connection *conn,
                                uint64_t stream_id,
                                uint8_t urgency,
                                bool incremental);

int quiche_conn_stream_shutdown(struct Connection *conn,
                                uint64_t stream_id,
                                enum Shutdown direction,
                                uint64_t err);

ssize_t quiche_conn_stream_capacity(const struct Connection *conn, uint64_t stream_id);

bool quiche_conn_stream_readable(const struct Connection *conn, uint64_t stream_id);

int64_t quiche_conn_stream_readable_next(struct Connection *conn);

int quiche_conn_stream_writable(struct Connection *conn, uint64_t stream_id, uintptr_t len);

int64_t quiche_conn_stream_writable_next(struct Connection *conn);

bool quiche_conn_stream_finished(const struct Connection *conn, uint64_t stream_id);

struct StreamIter *quiche_conn_readable(const struct Connection *conn);

struct StreamIter *quiche_conn_writable(const struct Connection *conn);

uintptr_t quiche_conn_max_send_udp_payload_size(const struct Connection *conn);

bool quiche_conn_is_readable(const struct Connection *conn);

int quiche_conn_close(struct Connection *conn,
                      bool app,
                      uint64_t err,
                      const uint8_t *reason,
                      size_t reason_len);

uint64_t quiche_conn_timeout_as_nanos(const struct Connection *conn);

uint64_t quiche_conn_timeout_as_millis(const struct Connection *conn);

void quiche_conn_on_timeout(struct Connection *conn);

void quiche_conn_trace_id(const struct Connection *conn, const uint8_t **out, size_t *out_len);

void quiche_conn_source_id(const struct Connection *conn, const uint8_t **out, size_t *out_len);

void quiche_conn_destination_id(const struct Connection *conn,
                                const uint8_t **out,
                                size_t *out_len);

void quiche_conn_application_proto(const struct Connection *conn,
                                   const uint8_t **out,
                                   size_t *out_len);

void quiche_conn_peer_cert(const struct Connection *conn, const uint8_t **out, size_t *out_len);

void quiche_conn_session(const struct Connection *conn, const uint8_t **out, size_t *out_len);

bool quiche_conn_is_established(const struct Connection *conn);

bool quiche_conn_is_in_early_data(const struct Connection *conn);

bool quiche_conn_is_draining(const struct Connection *conn);

bool quiche_conn_is_closed(const struct Connection *conn);

bool quiche_conn_is_timed_out(const struct Connection *conn);

bool quiche_conn_peer_error(const struct Connection *conn,
                            bool *is_app,
                            uint64_t *error_code,
                            const uint8_t **reason,
                            size_t *reason_len);

bool quiche_conn_local_error(const struct Connection *conn,
                             bool *is_app,
                             uint64_t *error_code,
                             const uint8_t **reason,
                             size_t *reason_len);

bool quiche_stream_iter_next(struct StreamIter *iter, uint64_t *stream_id);

void quiche_stream_iter_free(struct StreamIter *iter);

void quiche_conn_stats(const struct Connection *conn, struct Stats *out);

bool quiche_conn_peer_transport_params(const struct Connection *conn, struct TransportParams *out);

int quiche_conn_path_stats(const struct Connection *conn, uintptr_t idx, struct PathStats *out);

bool quiche_conn_is_server(const struct Connection *conn);

ssize_t quiche_conn_dgram_max_writable_len(const struct Connection *conn);

ssize_t quiche_conn_dgram_recv_front_len(const struct Connection *conn);

ssize_t quiche_conn_dgram_recv_queue_len(const struct Connection *conn);

ssize_t quiche_conn_dgram_recv_queue_byte_size(const struct Connection *conn);

ssize_t quiche_conn_dgram_send_queue_len(const struct Connection *conn);

ssize_t quiche_conn_dgram_send_queue_byte_size(const struct Connection *conn);

ssize_t quiche_conn_dgram_send(struct Connection *conn, const uint8_t *buf, size_t buf_len);

ssize_t quiche_conn_dgram_recv(struct Connection *conn, uint8_t *out, size_t out_len);

void quiche_conn_dgram_purge_outgoing(struct Connection *conn, bool (*f)(const uint8_t*, size_t));

ssize_t quiche_conn_send_ack_eliciting(struct Connection *conn);

ssize_t quiche_conn_send_ack_eliciting_on_path(struct Connection *conn,
                                               const struct sockaddr *local,
                                               socklen_t local_len,
                                               const struct sockaddr *peer,
                                               socklen_t peer_len);

void quiche_conn_free(struct Connection *conn);

uint64_t quiche_conn_peer_streams_left_bidi(const struct Connection *conn);

uint64_t quiche_conn_peer_streams_left_uni(const struct Connection *conn);

size_t quiche_conn_send_quantum(const struct Connection *conn);

int quiche_put_varint(uint8_t *buf, size_t buf_len, uint64_t val);

ssize_t quiche_get_varint(const uint8_t *buf, size_t buf_len, uint64_t *val);

struct Config *quiche_h3_config_new(void);

void quiche_h3_config_set_max_field_section_size(struct Config *config, uint64_t v);

void quiche_h3_config_set_qpack_max_table_capacity(struct Config *config, uint64_t v);

void quiche_h3_config_set_qpack_blocked_streams(struct Config *config, uint64_t v);

void quiche_h3_config_enable_extended_connect(struct Config *config, bool enabled);

void quiche_h3_config_free(struct Config *config);

struct Connection *quiche_h3_conn_new_with_transport(struct Connection *quic_conn,
                                                     struct Config *config);

int quiche_h3_for_each_setting(const struct Connection *conn, int (*cb)(uint64_t identifier,
                                                                        uint64_t value,
                                                                        void *argp), void *argp);

int64_t quiche_h3_conn_poll(struct Connection *conn,
                            struct Connection *quic_conn,
                            const struct Event **ev);

uint32_t quiche_h3_event_type(const struct Event *ev);

int quiche_h3_event_for_each_header(const struct Event *ev, int (*cb)(const uint8_t *name,
                                                                      size_t name_len,
                                                                      const uint8_t *value,
                                                                      size_t value_len,
                                                                      void *argp), void *argp);

bool quiche_h3_event_headers_has_body(const struct Event *ev);

bool quiche_h3_extended_connect_enabled_by_peer(const struct Connection *conn);

void quiche_h3_event_free(struct Event *ev);

int64_t quiche_h3_send_request(struct Connection *conn,
                               struct Connection *quic_conn,
                               const struct Header *headers,
                               size_t headers_len,
                               bool fin);

int quiche_h3_send_response(struct Connection *conn,
                            struct Connection *quic_conn,
                            uint64_t stream_id,
                            const struct Header *headers,
                            size_t headers_len,
                            bool fin);

int quiche_h3_send_response_with_priority(struct Connection *conn,
                                          struct Connection *quic_conn,
                                          uint64_t stream_id,
                                          const struct Header *headers,
                                          size_t headers_len,
                                          const struct Priority *priority,
                                          bool fin);

ssize_t quiche_h3_send_body(struct Connection *conn,
                            struct Connection *quic_conn,
                            uint64_t stream_id,
                            const uint8_t *body,
                            size_t body_len,
                            bool fin);

ssize_t quiche_h3_recv_body(struct Connection *conn,
                            struct Connection *quic_conn,
                            uint64_t stream_id,
                            uint8_t *out,
                            size_t out_len);

int quiche_h3_parse_extensible_priority(const uint8_t *priority,
                                        size_t priority_len,
                                        struct Priority *parsed);

int quiche_h3_send_priority_update_for_request(struct Connection *conn,
                                               struct Connection *quic_conn,
                                               uint64_t stream_id,
                                               const struct Priority *priority);

int quiche_h3_take_last_priority_update(struct Connection *conn,
                                        uint64_t prioritized_element_id,
                                        int (*cb)(const uint8_t *priority_field_value,
                                                  size_t priority_field_value_len,
                                                  void *argp),
                                        void *argp);

bool quiche_h3_dgram_enabled_by_peer(const struct Connection *conn,
                                     const struct Connection *quic_conn);

void quiche_h3_conn_free(struct Connection *conn);

#if defined(__cplusplus)
}  // extern C
#endif
#endif
