/*
 * iibin_registry.c – Registry Lifecycle & Thread-Safety Utilities
 * ==============================================================
 *
 * Verwaltet die globalen HashTables für Schemas und Enums.
 * Achtet auf ZTS-Kompatibilität und Latenzarmes Locking.
 */
#include "php_quicpro.h"
#include "iibin_internal.h"
#include <Zend/zend_smart_str.h>

#ifdef ZTS
# include <TSRM.h>
static MUTEX_T registry_mutex = NULL;
# define REG_LOCK()   tsrm_mutex_lock(registry_mutex)
# define REG_UNLOCK() tsrm_mutex_unlock(registry_mutex)
#else
# define REG_LOCK()   /* noop */
# define REG_UNLOCK() /* noop */
#endif

/* --------------------------------------------------------------
 *  Public helpers (exportiert via iibin_internal.h)
 * ------------------------------------------------------------*/
const quicpro_iibin_compiled_schema_internal*
get_compiled_iibin_schema_internal(const char *name)
{
    REG_LOCK();
    const quicpro_iibin_compiled_schema_internal *ptr =
        zend_hash_str_find_ptr(&quicpro_iibin_schema_registry, name, strlen(name));
    REG_UNLOCK();
    return ptr;
}

const quicpro_iibin_compiled_enum_internal*
get_compiled_iibin_enum_internal(const char *name)
{
    REG_LOCK();
    const quicpro_iibin_compiled_enum_internal *ptr =
        zend_hash_str_find_ptr(&quicpro_iibin_enum_registry, name, strlen(name));
    REG_UNLOCK();
    return ptr;
}

/* --------------------------------------------------------------
 *  MINIT / MSHUTDOWN
 * ------------------------------------------------------------*/
int quicpro_iibin_registries_init(void)
{
#ifdef ZTS
    if (!registry_mutex) registry_mutex = tsrm_mutex_alloc();
#endif
    zend_hash_init(&quicpro_iibin_schema_registry, 8, NULL,
                   quicpro_iibin_compiled_schema_dtor_internal, 1);
    zend_hash_init(&quicpro_iibin_enum_registry,   8, NULL,
                   quicpro_iibin_compiled_enum_dtor_internal, 1);
    quicpro_iibin_registries_initialized = 1;
    return SUCCESS;
}

void quicpro_iibin_registries_shutdown(void)
{
    if (!quicpro_iibin_registries_initialized) return;
    zend_hash_destroy(&quicpro_iibin_schema_registry);
    zend_hash_destroy(&quicpro_iibin_enum_registry);
    quicpro_iibin_registries_initialized = 0;
#ifdef ZTS
    if (registry_mutex) { tsrm_mutex_free(registry_mutex); registry_mutex = NULL; }
#endif
}
