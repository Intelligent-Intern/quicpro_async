<?php
$ffi = FFI::cdef("
    typedef struct quiche_config quiche_config;
    typedef struct quiche_conn   quiche_conn;
    typedef struct quiche_h3_config quiche_h3_config;
    typedef struct quiche_h3_conn   quiche_h3_conn;

    quiche_config *quiche_config_new(uint8_t version);
    void quiche_config_set_initial_max_data(quiche_config *config, uint64_t v);
    void quiche_config_set_initial_max_stream_data_bidi_local(quiche_config *config, uint64_t v);
    void quiche_config_set_initial_max_streams_bidi(quiche_config *config, uint64_t v);

    quiche_conn *quiche_connect(const char *server_name,
                                const uint8_t *scid, size_t scid_len,
                                quiche_config *config);

    ssize_t quiche_conn_send(quiche_conn *conn, uint8_t *out, size_t out_len,
                             const struct sockaddr *to, socklen_t to_len);

    ssize_t quiche_conn_recv(quiche_conn *conn, uint8_t *buf, size_t buf_len,
                             struct sockaddr *from, socklen_t *from_len);

    void        quiche_conn_free(quiche_conn *conn);

    // …and any other quiche_*/quiche_h3_* functions you need…
", "/usr/local/lib/libquiche.so");

class QuicProSession {
    private $config;
    private $conn;
    private $sock;
    private $destAddr;
    private $destPort;

    public function __construct(string $host, int $port) {
        // UDP socket
        $this->sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
        socket_set_nonblock($this->sock);
        $this->destAddr = $host;
        $this->destPort = $port;

        // Build a quiche_config
        $this->config = $GLOBALS['ffi']->quiche_config_new(0x1);
        $GLOBALS['ffi']->quiche_config_set_initial_max_data($this->config, 10_000_000);
        $GLOBALS['ffi']->quiche_config_set_initial_max_stream_data_bidi_local($this->config, 1_000_000);
        $GLOBALS['ffi']->quiche_config_set_initial_max_streams_bidi($this->config, 100);

        // Create the quiche_conn
        $scid = random_bytes(16);
        $this->conn = $GLOBALS['ffi']->quiche_connect(
            $host,
            FFI::addr(FFI::new("uint8_t[16]", false, FFI::addr(FFI::new("uint8_t[16]", false)))), 
            16,
            $this->config
        );
    }

    public function sendInitial(): void {
        $out = FFI::new("uint8_t[65535]");
        $written = $GLOBALS['ffi']->quiche_conn_send(
            $this->conn,
            $out,
            65535,
            FFI::addr(FFI::new("struct sockaddr_in")),
            FFI::addr(FFI::new("socklen_t"))
        );
        // send over UDP
        socket_sendto($this->sock, FFI::string($out, $written), $written, 0, $this->destAddr, $this->destPort);
    }

    public function recvLoop(): void {
        $buf = FFI::new("uint8_t[65535]");
        while (true) {
            $from = FFI::new("struct sockaddr_in");
            $fromlen = FFI::new("socklen_t");
            $n = socket_recvfrom($this->sock, $data, 65535, 0, $ip, $port);
            if ($n > 0) {
                // copy into FFI buffer
                FFI::memcpy($buf, $data, $n);
                $GLOBALS['ffi']->quiche_conn_recv(
                    $this->conn,
                    $buf,
                    65535,
                    FFI::addr($from),
                    FFI::addr($fromlen)
                );
            }
            // check state, break if established, etc…
            break;
        }
    }

    public function close(): void {
        $GLOBALS['ffi']->quiche_conn_free($this->conn);
        socket_close($this->sock);
    }
}

// Usage:
$s = new QuicProSession("example.com", 4433);
$s->sendInitial();
$s->recvLoop();
$s->close();

