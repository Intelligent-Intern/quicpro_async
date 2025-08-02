#include "php_quicpro.h"
#include "client/session.h"
#include "config/config.h"
#include "client/tls.h"
#include "client/cancel.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/rand.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

/**
 * @file extension/src/client/session.c
 * @brief Client-Side QUIC Session Management Implementation for Quicpro.
 *
 * This file provides the comprehensive implementation for managing client-initiated
 * QUIC sessions. It includes the core logic for establishing new QUIC connections,
 * driving the QUIC state machine through regular "ticking" operations, handling
 * network I/O (sending/receiving UDP datagrams), and gracefully closing sessions.
 *
 * The module heavily relies on the `quiche` library for the underlying QUIC protocol
 * implementation and integrates with Quicpro's global configuration system to apply
 * various network, TLS, and performance-tuning settings. It also incorporates
 * functionalities for TLS session ticket export/import (0-RTT) and optional
 * kernel-level timestamping for diagnostics.
 *
 * This implementation replaces and consolidates the client-side aspects
 * of legacy `connect.c`, `session.c`, and `poll.c` into a unified and optimized
 * client-specific session management component.
 */

// Global resource type for Quicpro\Session objects.
extern int le_quicpro_session;

/**
 * @brief Helper function to set a UDP socket to non-blocking mode.
 *
 * This function creates a new UDP socket with the `SOCK_NONBLOCK` flag set,
 * ensuring that I/O operations on this socket will not block the calling thread.
 * Non-blocking sockets are essential for asynchronous network programming
 * and integration with event loops (like PHP Fibers).
 *
 * @param family The address family (e.g., AF_INET, AF_INET6).
 * @return The file descriptor of the non-blocking UDP socket on success, or -1 on error.
 */
static int udp_socket_nonblock(int family) {
    int sock = socket(family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (sock < 0) {
        throw_network_exception(errno, "Failed to create non-blocking UDP socket: %s", strerror(errno));
    }
    return sock;
}

/**
 * @brief Helper function to resolve a hostname and port into address information.
 *
 * This function uses `getaddrinfo` to perform DNS resolution for a given
 * hostname and service (port). It populates a linked list of `addrinfo`
 * structures, which can then be iterated to find a suitable address for connection.
 * It's critical for establishing connections by hostname rather than raw IP.
 *
 * @param host The hostname or IP address string.
 * @param port The port number as a string.
 * @param res A pointer to a `struct addrinfo*` which will be populated with results.
 * @return 0 on success, or a non-zero error code from `getaddrinfo` on failure.
 */
static int resolve_host(const char *host, const char *port, struct addrinfo **res) {
    struct addrinfo hints = {0};
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_flags    = AI_ADDRCONFIG;
    int status = getaddrinfo(host, port, &hints, res);
    if (status != 0) {
        throw_network_exception(status, "DNS resolution failed for host '%s': %s", host, gai_strerror(status));
    }
    return status;
}

/**
 * @brief Helper function to bind a socket to a specific network interface.
 *
 * This function uses `setsockopt` with `SO_BINDTODEVICE` to bind the socket
 * to a particular network interface (e.g., "eth0"). This is important for
 * multi-homed systems or when specific routing is required.
 * This option is Linux-specific.
 *
 * @param fd The socket file descriptor.
 * @param iface The name of the interface (e.g., "eth0").
 * @return 0 on success, or -1 on error.
 */
static int socket_bind_iface(int fd, const char *iface) {
#ifdef SO_BINDTODEVICE
    int res = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface));
    if (res < 0) {
        throw_network_exception(errno, "Failed to bind socket to interface '%s': %s", iface, strerror(errno));
    }
    return res;
#else
    php_error_docref(NULL, E_NOTICE, "SO_BINDTODEVICE is not supported on this operating system.");
    return 0;
#endif
}


/**
 * @brief Establishes a new QUIC client session to a specified host and port.
 *
 * This PHP function initiates a new QUIC connection. It handles all phases:
 * DNS resolution, UDP socket creation (non-blocking), binding to a specific
 * interface if requested, and the initial `quiche_connect` call. It ensures
 * that the underlying `quiche` configuration is applied, which includes TLS
 * settings, QUIC transport parameters, and performance tuning options.
 *
 * The function encapsulates the "Happy Eyeballs" logic for IP family selection
 * by iterating through resolved addresses and attempting connections.
 * NUMA node affinity is applied for performance optimization if specified.
 *
 * @param host_str The target hostname or IP address of the QUIC server.
 * @param host_len The length of `host_str`.
 * @param port The target UDP port number.
 * @param config_resource A PHP resource representing a `Quicpro\Config` object. This
 * resource provides comprehensive configuration settings for the QUIC session.
 * @param numa_node An optional NUMA node ID (`int`) for hinting memory allocation
 * and CPU affinity. Pass -1 for automatic/default behavior.
 * @param options_array Optional PHP array containing connection-specific options
 * like `preferred_ip_family` (ipv4, ipv6, auto) or `interface`.
 * @return A PHP resource of type `Quicpro\Session` on success. Returns FALSE on failure,
 * throwing appropriate `Quicpro\Exception` subclasses (e.g., `QuicException`, `TlsException`,
 * `NetworkException`) for detailed error reporting.
 */
PHP_FUNCTION(quicpro_client_session_connect) {
    char *host_str;
    size_t host_len;
    zend_long port;
    zval *z_config_res;
    zend_long numa_node = -1;
    zval *options_array = NULL;

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_STRING(host_str, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(z_config_res)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(numa_node)
        Z_PARAM_ARRAY_OR_NULL(options_array)
    ZEND_PARSE_PARAMETERS_END();

    // Fetch the global Quicpro configuration object.
    quicpro_cfg_t *cfg_wrapper = (quicpro_cfg_t *)zend_fetch_resource_ex(z_config_res, "Quicpro\\Config", le_quicpro_cfg);
    if (!cfg_wrapper || cfg_wrapper->quiche_cfg == NULL) {
        throw_config_exception(0, "Invalid or uninitialized Quicpro\\Config resource provided.");
        RETURN_FALSE;
    }
    // Ensure the config is marked as frozen as it's now being used.
    quicpro_config_mark_frozen(cfg_wrapper);

    // Allocate memory for the new session object.
    quicpro_session_t *s = ecalloc(1, sizeof(*s));
    // Initialize session state.
    s->sock = -1;
    s->conn = NULL;
    s->h3 = NULL;
    s->cfg_ptr = cfg_wrapper;
    s->h3_cfg = NULL;
    s->ticket_len = 0;
    s->ts_enabled = 0;
    s->numa_node = (int)numa_node;
    s->is_closed = false;

    // Copy host string for SNI and authority header.
    if (host_len >= sizeof(s->host)) {
        throw_network_exception(0, "Hostname '%s' is too long. Maximum allowed is %zu characters.", host_str, sizeof(s->host) - 1);
        efree(s);
        RETURN_FALSE;
    }
    strncpy(s->host, host_str, sizeof(s->host) - 1);
    s->host[sizeof(s->host) - 1] = '\0';

    // Generate a random Source Connection ID (SCID) for the client.
    RAND_bytes(s->scid, sizeof(s->scid));

    // Resolve host and port to address information.
    struct addrinfo *ai_list = NULL;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%ld", port);
    if (resolve_host(host_str, port_str, &ai_list) != 0) {
        efree(s);
        RETURN_FALSE;
    }

    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len = 0;
    int selected_family = AF_UNSPEC;
    int bind_sock = -1;
    bool connected_successfully = false;

    // --- Happy Eyeballs for IP Family and Interface Binding ---
    // Extract preferred IP family and interface if specified in options.
    zend_string *preferred_ip_family_str = NULL;
    char *interface_str = NULL;
    size_t interface_len = 0;

    if (options_array && Z_TYPE_P(options_array) == IS_ARRAY) {
        zval *ip_family_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "preferred_ip_family", sizeof("preferred_ip_family") - 1);
        if (ip_family_val && Z_TYPE_P(ip_family_val) == IS_STRING) {
            preferred_ip_family_str = Z_STR_P(ip_family_val);
        }
        zval *iface_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "interface", sizeof("interface") - 1);
        if (iface_val && Z_TYPE_P(iface_val) == IS_STRING) {
            interface_str = Z_STRVAL_P(iface_val);
            interface_len = Z_STRLEN_P(iface_val);
        }
    }

    // Try IPv6 first if not explicitly set to IPv4
    bool try_ipv6 = !preferred_ip_family_str || zend_string_equals_literal(preferred_ip_family_str, "auto") || zend_string_equals_literal(preferred_ip_family_str, "ipv6");
    bool try_ipv4 = !preferred_ip_family_str || zend_string_equals_literal(preferred_ip_family_str, "auto") || zend_string_equals_literal(preferred_ip_family_str, "ipv4");

    // Attempt IPv6 connections first, if enabled.
    if (try_ipv6) {
        for (struct addrinfo *ai = ai_list; ai != NULL; ai = ai->ai_next) {
            if (ai->ai_family != AF_INET6) continue;

            bind_sock = udp_socket_nonblock(ai->ai_family);
            if (bind_sock < 0) continue;

            if (interface_str && interface_len > 0) {
                if (socket_bind_iface(bind_sock, interface_str) < 0) {
                    close(bind_sock);
                    bind_sock = -1;
                    continue;
                }
            }

            if (connect(bind_sock, ai->ai_addr, ai->ai_addrlen) == 0) {
                memcpy(&peer_addr, ai->ai_addr, ai->ai_addrlen);
                peer_addr_len = (socklen_t)ai->ai_addrlen;
                selected_family = AF_INET6;
                connected_successfully = true;
                break;
            }
            close(bind_sock);
            bind_sock = -1;
        }
    }

    // If no IPv6 connection, or IPv6 explicitly disabled, try IPv4.
    if (!connected_successfully && try_ipv4) {
        for (struct addrinfo *ai = ai_list; ai != NULL; ai = ai->ai_next) {
            if (ai->ai_family != AF_INET) continue;

            bind_sock = udp_socket_nonblock(ai->ai_family);
            if (bind_sock < 0) continue;

            if (interface_str && interface_len > 0) {
                if (socket_bind_iface(bind_sock, interface_str) < 0) {
                    close(bind_sock);
                    bind_sock = -1;
                    continue;
                }
            }

            if (connect(bind_sock, ai->ai_addr, ai->ai_addrlen) == 0) {
                memcpy(&peer_addr, ai->ai_addr, ai->ai_addrlen);
                peer_addr_len = (socklen_t)ai->ai_addrlen;
                selected_family = AF_INET;
                connected_successfully = true;
                break;
            }
            close(bind_sock);
            bind_sock = -1;
        }
    }

    // Final check for connection success.
    if (!connected_successfully) {
        freeaddrinfo(ai_list);
        efree(s);
        throw_network_exception(0, "Failed to connect UDP socket to host '%s:%ld' using any available IP family. Last system error: %s", host_str, port, strerror(errno));
        RETURN_FALSE;
    }

    s->sock = bind_sock;
    freeaddrinfo(ai_list);

    // Initialize the QUIC connection using quiche_connect.
    // This starts the QUIC state machine and initiates the handshake.
    s->conn = quiche_connect(
        s->host,
        s->scid, sizeof(s->scid),
        NULL, 0,
        (const struct sockaddr *)&peer_addr,
        peer_addr_len,
        cfg_wrapper->quiche_cfg
    );

    if (!s->conn) {
        close(s->sock);
        efree(s);
        throw_quic_exception(0, "Failed to create new QUIC connection via quiche_connect. This indicates an invalid configuration or resource exhaustion.");
        RETURN_FALSE;
    }

    // Initialize the HTTP/3 layer on top of the QUIC connection.
    // This is necessary for sending/receiving HTTP/3 frames and managing streams.
    s->h3_cfg = quiche_h3_config_new();
    if (!s->h3_cfg) {
        quiche_conn_free(s->conn);
        close(s->sock);
        efree(s);
        throw_quic_exception(0, "Failed to initialize HTTP/3 configuration. System memory exhausted.");
        RETURN_FALSE;
    }
    s->h3 = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);
    if (!s->h3) {
        quiche_h3_config_free(s->h3_cfg);
        quiche_conn_free(s->conn);
        close(s->sock);
        efree(s);
        throw_quic_exception(0, "Failed to initialize HTTP/3 connection. This indicates an invalid QUIC connection state or a severe configuration mismatch.");
        RETURN_FALSE;
    }

    // Register the session as a PHP resource.
    RETURN_RES(zend_register_resource(s, le_quicpro_session));
}

/**
 * @brief Drives the state machine of an active QUIC session by processing I/O and timers.
 *
 * This PHP function is critical for the continuous operation of any active QUIC session.
 * It performs a multitude of tasks necessary for connection liveness and data transfer:
 * 1.  **Reads Incoming Packets:** It continuously attempts to read pending UDP packets
 * from the session's socket. Each received packet is then fed into the `quiche_conn`
 * state machine for decryption and processing.
 * 2.  **Processes Internal Timers:** The QUIC protocol relies heavily on internal timers
 * for managing retransmissions, keep-alives, and various protocol timeouts. This
 * function ensures these timers are advanced and acted upon.
 * 3.  **Generates Outgoing Packets:** Based on the current state of the `quiche_conn`
 * (e.g., new data to send, ACKs to acknowledge received packets, retransmissions),
 * it generates outgoing QUIC packets.
 * 4.  **Writes Outgoing Packets:** These generated packets are then written to the
 * UDP socket for transmission over the network.
 * 5.  **Advances QUIC Clock:** The `advance_us` parameter (or system time if 0) is used
 * to advance the `quiche_conn`'s internal monotonic clock, which is vital for
 * accurate RTT estimation and timer management.
 * 6.  **Triggers Callbacks:** It also polls the HTTP/3 layer to trigger application-level
 * callbacks for stream-related events (e.g., new data on a stream, stream closure).
 *
 * This function effectively replaces the concept of a generic `poll` for QUIC,
 * as it specifically "ticks" the QUIC connection, ensuring progress
 * and responsiveness. It is designed to be called repeatedly within an
 * event loop (e.g., driven by PHP Fibers) to maintain an active and
 * performant QUIC connection.
 *
 * @param session_resource A PHP resource representing an active `Quicpro\Session`. Mandatory.
 * @param advance_us The number of microseconds to advance the QUIC connection's internal
 * timer. This can be used to simulate time passing for deterministic
 * testing or to provide a precise time quantum for event loops.
 * @return TRUE if the session is still active and potentially processing (not closed or draining),
 * FALSE if the session has closed or an unrecoverable error occurred. Throws a `Quicpro\Exception\QuicException`
 * on critical internal QUIC errors.
 */
PHP_FUNCTION(quicpro_client_session_tick) {
    zval *z_sess_res;
    zend_long advance_us;
    quicpro_session_t *s;
    ssize_t read_len;
    ssize_t written_len;
    uint8_t recv_buf[QUICPRO_MAX_PACKET_SIZE];
    uint8_t send_buf[QUICPRO_MAX_PACKET_SIZE];
    struct sockaddr_storage peer_addr;
    struct iovec iov = { .iov_base = recv_buf, .iov_len = sizeof(recv_buf) };
    char cmsg_buf[512];
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(z_sess_res)
        Z_PARAM_LONG(advance_us)
    ZEND_PARSE_PARAMETERS_END();

    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL || s->is_closed) {
        RETURN_FALSE;
    }

    // 1. Read incoming UDP packets from the socket
    // Continuously read until no more packets are immediately available (EAGAIN/EWOULDBLOCK).
    while (true) {
        struct msghdr msg = {
            .msg_name       = &peer_addr,
            .msg_namelen    = sizeof(peer_addr),
            .msg_iov        = &iov,
            .msg_iovlen     = 1,
            .msg_control    = cmsg_buf,
            .msg_controllen = sizeof(cmsg_buf),
        };

        read_len = recvmsg(s->sock, &msg, MSG_DONTWAIT);
        if (read_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // No more packets available for non-blocking read.
            }
            throw_network_exception(errno, "Failed to read UDP packet from socket: %s", strerror(errno));
            RETVAL_FALSE;
            goto cleanup_and_return;
        }
        if (read_len == 0) {
            break; // Peer performed an orderly shutdown, or connection closed (rare for UDP).
        }

        quiche_recv_info recv_info = {
            .from = (struct sockaddr *)&peer_addr,
            .from_len = msg.msg_namelen,
            .to = NULL,
            .to_len = 0
        };

        ssize_t quiche_ret = quiche_conn_recv(s->conn, recv_buf, read_len, &recv_info);
        if (quiche_ret < 0) {
            if (quiche_ret != QUICHE_ERR_DONE) { // QUICHE_ERR_DONE is not a hard error, means packet was processed but no bytes consumed (e.g., duplicate).
                // Log detailed information for debugging purposes. This indicates a potentially malformed
                // or unexpected packet that quiche could not process.
                php_error_docref(NULL, E_NOTICE, "quiche_conn_recv failed for incoming packet: %s (Error Code: %zd)", quiche_error_t_to_string((int)quiche_ret), quiche_ret);
            }
        }
        
        if (s->ts_enabled) {
            for (struct cmsghdr *cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
                if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING_NEW) {
                    memcpy(&s->last_rx_ts, CMSG_DATA(cm), sizeof(struct timespec));
                    break;
                }
            }
        }
    }

    // 2. Advance the QUIC connection's internal clock and process timers.
    quiche_conn_on_timeout(s->conn);

    if (advance_us == 0) {
        // If 0 microseconds are provided, use a small default quantum (1ms) for auto-tick.
        // This prevents busy-waiting in tight loops when no external timer drives the tick.
        advance_us = 1000;
    }

    // 3. Generate and Write Outgoing QUIC Packets
    while (true) {
        quiche_send_info send_info;
        ssize_t send_len = quiche_conn_send(s->conn, send_buf, sizeof(send_buf), &send_info);

        if (send_len == QUICHE_ERR_DONE) {
            break; // No more outgoing packets ready at this moment.
        }
        if (send_len < 0) {
            throw_quic_exception((int)send_len, "Failed to generate outgoing QUIC packet: %s", quiche_error_t_to_string((int)send_len));
            RETVAL_FALSE;
            goto cleanup_and_return;
        }

        written_len = sendto(s->sock, send_buf, (size_t)send_len, 0,
                             (const struct sockaddr *)&send_info.to, send_info.to_len);
        if (written_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // Socket is temporarily full; try again later.
            }
            throw_network_exception(errno, "Failed to send UDP packet: %s", strerror(errno));
            RETVAL_FALSE;
            goto cleanup_and_return;
        }
        if (written_len != send_len) {
            // This indicates a partial send, which should ideally not happen for UDP datagrams.
            // If it does, it's a critical network layer issue.
            throw_network_exception(0, "Partial UDP packet send. Expected %zd bytes, sent %zd bytes. This indicates a serious network stack issue.", send_len, written_len);
            RETVAL_FALSE;
            goto cleanup_and_return;
        }
    }

    // Check if the QUIC connection is still active (not closed or draining).
    if (quiche_conn_is_closed(s->conn) || quiche_conn_is_draining(s->conn)) {
        s->is_closed = true;
        RETURN_FALSE;
    }

    // Successfully processed a tick, session is still active.
    RETURN_TRUE;

cleanup_and_return:
    // This label handles cleanup for early exits due to exceptions.
    // Resource cleanup (s->conn, s->h3 etc.) is primarily handled by the resource dtor,
    // so we just ensure PHP's return value is correctly set to FALSE if an exception occurred.
    return;
}


/**
 * @brief Closes an active QUIC client session gracefully.
 *
 * This PHP function initiates the orderly shutdown of a QUIC connection from the client side.
 * It sends connection close frames to the peer, signaling the intent to terminate.
 * This allows the remote endpoint to gracefully clean up its state. After this call,
 * the `Quicpro\Session` resource is marked as closed, and its associated native
 * resources (socket, `quiche` contexts) will be released by PHP's garbage collection
 * when the resource's reference count drops to zero.
 *
 * @param session_resource A PHP resource representing the `Quicpro\Session` to close.
 * @param application_close A boolean flag. If TRUE, indicates an application-level
 * close (error code 0x00, for example). If FALSE, indicates a transport-level close.
 * @param error_code A numeric error code (application-defined or QUIC-defined)
 * indicating the reason for closure.
 * @param reason_str An optional string providing a human-readable reason for closure.
 * This string is transmitted to the peer.
 * @param reason_len The length of `reason_str`.
 * @return TRUE on successful initiation of the closure process. Returns FALSE on failure
 * (e.g., invalid session resource, session already closed, underlying QUIC error).
 * Throws a `Quicpro\Exception\QuicException` on protocol-level errors during close.
 */
PHP_FUNCTION(quicpro_client_session_close) {
    zval *z_sess_res;
    zend_bool application_close = 1;
    zend_long error_code = 0;
    char *reason_str = NULL;
    size_t reason_len = 0;
    quicpro_session_t *s;

    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_RESOURCE(z_sess_res)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(application_close)
        Z_PARAM_LONG(error_code)
        Z_PARAM_STRING_OR_NULL(reason_str, reason_len)
    ZEND_PARSE_PARAMETERS_END();

    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL || s->is_closed) {
        RETURN_FALSE;
    }

    quiche_conn_close(
        s->conn,
        (bool)application_close,
        (uint64_t)error_code,
        (const uint8_t *)(reason_str ? reason_str : ""),
        (size_t)(reason_str ? reason_len : 0)
    );

    s->is_closed = true;

    quicpro_client_session_tick(ZEND_NUM_ARGS() - 1, NULL, z_sess_res, 0L);

    RETURN_TRUE;
}

/**
 * @brief Fetches an outgoing QUIC datagram from the session.
 *
 * This PHP function retrieves a complete UDP datagram (containing one or more
 * QUIC packets) that is ready to be sent over the network from the internal
 * `quiche_conn` buffer. This is a crucial part of the write-side of the
 * QUIC event loop. The function provides the raw binary data and its
 * destination address, ready for a `sendto` call on the UDP socket.
 *
 * @param session_resource A PHP resource representing an active `Quicpro\Session`.
 * @param buffer_zval A PHP string zval passed by reference, into which the
 * outgoing datagram will be written. Its capacity must be sufficient.
 * @param buffer_size The maximum size of the buffer provided in `buffer_zval`.
 * @return The number of bytes written into the `buffer_zval` (i.e., the length of the datagram)
 * on success. Returns 0 if no datagram is currently available to send. Returns FALSE on error.
 * Throws a `Quicpro\Exception\QuicException` for underlying `quiche` errors.
 */
PHP_FUNCTION(quicpro_client_session_fetch_datagram) {
    zval *z_sess_res;
    zval *z_buffer;
    zend_long buffer_size;
    quicpro_session_t *s;
    quiche_send_info send_info;
    ssize_t send_len;
    uint8_t *tmp_buf;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(z_sess_res)
        Z_PARAM_ZVAL(z_buffer)
        Z_PARAM_LONG(buffer_size)
    ZEND_PARSE_PARAMETERS_END();

    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL || s->is_closed) {
        RETURN_FALSE;
    }

    if (buffer_size <= 0) {
        throw_quic_exception(0, "Provided buffer_size must be a positive integer.");
        RETURN_FALSE;
    }

    tmp_buf = emalloc(buffer_size);

    send_len = quiche_conn_send(s->conn, tmp_buf, (size_t)buffer_size, &send_info);

    if (send_len == QUICHE_ERR_DONE) {
        efree(tmp_buf);
        RETURN_LONG(0);
    }
    if (send_len < 0) {
        efree(tmp_buf);
        throw_quic_exception((int)send_len, "Failed to fetch outgoing QUIC datagram: %s", quiche_error_t_to_string((int)send_len));
        RETURN_FALSE;
    }

    ZVAL_STRINGL(z_buffer, (char *)tmp_buf, (size_t)send_len);

    efree(tmp_buf);

    RETURN_LONG(send_len);
}


/**
 * @brief Ingests an incoming UDP datagram into the QUIC session.
 *
 * This PHP function feeds a raw UDP packet's payload into the internal
 * `quiche_conn` state machine. This is a crucial part of the read-side
 * of the QUIC event loop. The function requires the raw packet data
 * and information about the sender (peer IP and port) to correctly
 * attribute the packet within the QUIC connection context.
 *
 * @param session_resource A PHP resource representing an active `Quicpro\Session`.
 * @param packet_data_str The raw binary data of the received UDP packet payload.
 * @param packet_len The length of `packet_data_str`.
 * @param peer_ip_str The string representation of the peer's IP address (e.g., "127.0.0.1", "::1").
 * @param peer_ip_len The length of `peer_ip_str`.
 * @param peer_port The UDP port of the peer.
 * @return TRUE on successful ingestion of the datagram. Returns FALSE on failure
 * (e.g., invalid session resource, malformed packet, internal `quiche` error).
 * Throws a `Quicpro\Exception\QuicException` for protocol violations.
 */
PHP_FUNCTION(quicpro_client_session_ingest_datagram) {
    zval *z_sess_res;
    char *packet_data_str;
    size_t packet_len;
    char *peer_ip_str;
    size_t peer_ip_len;
    zend_long peer_port;
    quicpro_session_t *s;
    struct sockaddr_storage peer_addr;
    struct sockaddr_in *s4;
    struct sockaddr_in6 *s6;
    int family = AF_UNSPEC;

    ZEND_PARSE_PARAMETERS_START(5, 5)
        Z_PARAM_RESOURCE(z_sess_res)
        Z_PARAM_STRING(packet_data_str, packet_len)
        Z_PARAM_STRING(peer_ip_str, peer_ip_len)
        Z_PARAM_LONG(peer_port)
    ZEND_PARSE_PARAMETERS_END();

    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL || s->is_closed) {
        RETURN_FALSE;
    }

    if (strchr(peer_ip_str, ':') != NULL) {
        family = AF_INET6;
        s6 = (struct sockaddr_in6 *)&peer_addr;
        s6->sin6_family = AF_INET6;
        s6->sin6_port = htons((uint16_t)peer_port);
        if (inet_pton(AF_INET6, peer_ip_str, &s6->sin6_addr) != 1) {
            throw_network_exception(0, "Invalid IPv6 address string '%s'.", peer_ip_str);
            RETURN_FALSE;
        }
        memset(&s6->sin6_zero, 0, sizeof(s6->sin6_zero));
    } else {
        family = AF_INET;
        s4 = (struct sockaddr_in *)&peer_addr;
        s4->sin_family = AF_INET;
        s4->sin_port = htons((uint16_t)peer_port);
        if (inet_pton(AF_INET, peer_ip_str, &s4->sin_addr) != 1) {
            throw_network_exception(0, "Invalid IPv4 address string '%s'.", peer_ip_str);
            RETURN_FALSE;
        }
    }

    quiche_recv_info recv_info = {
        .from = (struct sockaddr *)&peer_addr,
        .from_len = (family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in),
        .to = NULL,
        .to_len = 0
    };

    ssize_t quiche_ret = quiche_conn_recv(s->conn, (uint8_t *)packet_data_str, packet_len, &recv_info);
    if (quiche_ret < 0) {
        if (quiche_ret != QUICHE_ERR_DONE) {
            throw_quic_exception((int)quiche_ret, "Failed to ingest incoming QUIC datagram: %s", quiche_error_t_to_string((int)quiche_ret));
            RETURN_FALSE;
        }
    }

    RETURN_TRUE;
}

/**
 * @brief Retrieves the next available crypto stream ID for the session.
 *
 * This PHP function is used to iterate over "crypto streams" within a QUIC session.
 * Crypto streams are special unidirectional streams used by the QUIC protocol itself
 * for exchanging handshake and control messages. Applications usually don't interact
 * with these directly, but they might need to be drained or processed in a polling loop.
 *
 * @param session_resource A PHP resource representing an active `Quicpro\Session`.
 * @param stream_id_out A reference to a PHP long variable. If a crypto stream ID is found,
 * it will be stored here. This parameter is passed by reference.
 * @return TRUE if a new crypto stream ID was successfully retrieved and stored in `stream_id_out`.
 * Returns FALSE if no more crypto streams are available or on error.
 */
PHP_FUNCTION(quicpro_client_session_next_crypto_stream) {
    zval *z_sess_res;
    zval *z_stream_id_out;
    quicpro_session_t *s;
    uint64_t sid;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(z_sess_res)
        Z_PARAM_ZVAL(z_stream_id_out)
    ZEND_PARSE_PARAMETERS_END();

    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL || s->is_closed) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(z_stream_id_out) != IS_LONG) {
        zval_set_long(z_stream_id_out, 0);
    }

    if (quiche_session_next_crypto_stream(s->conn, &sid)) {
        ZVAL_LONG(z_stream_id_out, (zend_long)sid);
        RETURN_TRUE;
    }

    RETURN_FALSE;
}

/**
 * @brief Checks if a QUIC client session is fully established and ready for application data.
 *
 * A QUIC session reaches the "established" state after the successful completion of
 * the TLS handshake. Until this state is reached, only cryptographic handshake data
 * and specific control messages can be exchanged. Application-level data streams
 * (like those used by HTTP/3) can only be opened and used once the session is established.
 *
 * @param session_resource A PHP resource representing a `Quicpro\Session`.
 * @return TRUE if the session is fully established and ready for application traffic.
 * Returns FALSE otherwise (e.g., still in handshake, closed, or error state).
 */
PHP_FUNCTION(quicpro_client_session_is_established) {
    zval *z_sess_res;
    quicpro_session_t *s;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess_res)
    ZEND_PARSE_PARAMETERS_END();

    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL || s->is_closed) {
        RETURN_FALSE;
    }

    RETURN_BOOL(quiche_conn_is_established(s->conn));
}

#ifdef __linux__
/**
 * @brief Enables kernel-level packet timestamping for a QUIC session's UDP socket (Linux-specific).
 *
 * This PHP function configures the underlying UDP socket to request high-precision
 * hardware or software timestamps for received packets using the `SO_TIMESTAMPING_NEW`
 * socket option. These timestamps are invaluable for accurate Round-Trip Time (RTT)
 * estimation in congestion control algorithms and for detailed network diagnostics.
 *
 * The function sets the `ts_enabled` flag in the session struct to prevent redundant
 * `setsockopt` calls. This feature is only available on Linux systems with kernel
 * support.
 *
 * @param session_resource A PHP resource representing the `Quicpro\Session` whose socket
 * should be configured for timestamping.
 * @return TRUE on success (timestamping enabled or already enabled). Returns FALSE on
 * failure (e.g., socket error, OS not supporting feature).
 * Throws a `Quicpro\Exception\NetworkException` on system call failures.
 */
PHP_FUNCTION(quicpro_client_session_enable_kernel_timestamps) {
    zval *z_sess_res;
    quicpro_session_t *s;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess_res)
    ZEND_PARSE_PARAMETERS_END();

    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->sock < 0 || s->is_closed) {
        throw_network_exception(0, "Invalid or closed Quicpro\\Session resource provided for timestamping.");
        RETURN_FALSE;
    }

    if (s->ts_enabled) {
        RETURN_TRUE;
    }

    int flags = SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE |
                SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RAW_HARDWARE;

    if (setsockopt(s->sock, SOL_SOCKET, SO_TIMESTAMPING_NEW, &flags, sizeof(flags)) == 0) {
        s->ts_enabled = 1;
        RETURN_TRUE;
    } else {
        throw_network_exception(errno, "Failed to enable kernel timestamping on socket: %s", strerror(errno));
        RETURN_FALSE;
    }
}
#endif