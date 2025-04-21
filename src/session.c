#include "php_quicpro.h"
#include <openssl/rand.h>
#include <string.h>
#include <arpa/inet.h>

PHP_FUNCTION(quicpro_connect)
{
    char *host; size_t host_len; zend_long port;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sl", &host, &host_len, &port) == FAILURE)
        RETURN_FALSE;

    quicpro_session_t *s = ecalloc(1, sizeof(*s));

    s->cfg = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    quiche_config_set_application_protos(s->cfg, (uint8_t *)"\x05h3-29", 6);
    quiche_config_set_max_idle_timeout(s->cfg, 30000);
    quiche_config_set_max_send_udp_payload_size(s->cfg, 1350);

    uint8_t scid[16]; RAND_bytes(scid, sizeof(scid));

    s->sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_port   = htons((uint16_t)port);
    inet_pton(AF_INET, host, &peer.sin_addr);
    connect(s->sock, (struct sockaddr *)&peer, sizeof(peer));

    s->conn = quiche_connect(host,
                             scid, sizeof(scid),
                             NULL, 0,
                             (struct sockaddr *)&peer, sizeof(peer),
                             s->cfg);

    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);

    ZEND_REGISTER_RESOURCE(return_value, s, le_quicpro);
}

PHP_FUNCTION(quicpro_close)
{
    zval *z;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &z) == FAILURE)
        RETURN_FALSE;

    zend_list_close(Z_RES_P(z));
    RETURN_TRUE;
}