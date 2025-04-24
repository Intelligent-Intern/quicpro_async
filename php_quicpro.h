#ifndef PHP_QUICPRO_H
#define PHP_QUICPRO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

/* -------------------------------------------------------------------------
 *  PHP version compatibility (8.1 – 8.4)
 * ------------------------------------------------------------------------- */
#if PHP_VERSION_ID >= 80400 && PHP_VERSION_ID < 80500
#   define QUICPRO_PHP84_COMPAT 1
#endif

/* -------------------------------------------------------------------------
 *  Resource type identifiers
 * ------------------------------------------------------------------------- */

typedef enum {
    le_quicpro_session = 0,
    le_quicpro_cfg,
    le_quicpro_perf,
    le_quicpro__count /* sentinel – must be last */
} quicpro_resource_t;

/* -------------------------------------------------------------------------
 *  Thread‑local error handling helper
 * ------------------------------------------------------------------------- */
#ifndef QUICPRO_ERR_LEN
#   define QUICPRO_ERR_LEN 256
#endif

extern __thread char quicpro_last_error[QUICPRO_ERR_LEN];

static inline void quicpro_set_error(const char *msg)
{
    if (msg == NULL) {
        quicpro_last_error[0] = '\0';
        return;
    }
    strncpy(quicpro_last_error, msg, QUICPRO_ERR_LEN - 1);
    quicpro_last_error[QUICPRO_ERR_LEN - 1] = '\0';
}

/* -------------------------------------------------------------------------
 *  Shared‑memory session‑ticket cache (lock‑free ring)
 * ------------------------------------------------------------------------- */

#define QUICPRO_MAX_TICKET_SIZE 512

typedef struct {
    _Atomic uint64_t epoch;                 /* monotonically increasing version */
    uint32_t         len;                   /* ticket length in bytes          */
    uint8_t          data[QUICPRO_MAX_TICKET_SIZE];
} quicpro_ticket_entry_t;

/* lock‑free single‑producer/single‑consumer ring buffer */
typedef struct {
    _Atomic uint32_t head; /* write index */
    uint32_t         size; /* total number of entries  */
    quicpro_ticket_entry_t entries[]; /* flexible array */
} quicpro_ticket_ring_t;

/* -------------------------------------------------------------------------
 *  perf_event mmap page alias (for userspace tracing)
 * ------------------------------------------------------------------------- */
#ifdef __linux__
#   include <linux/perf_event.h>
    typedef struct perf_event_mmap_page quicpro_perf_page_t;
#else
    typedef struct { char _stub; } quicpro_perf_page_t; /* non‑Linux stub */
#endif

#endif /* PHP_QUICPRO_H */