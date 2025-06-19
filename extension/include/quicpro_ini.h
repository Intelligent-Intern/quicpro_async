/*
 * =========================================================================
 * FILENAME:   include/config/quicpro_ini.h
 * MODULE:     quicpro_async: Master PHP.ini Dispatcher
 * =========================================================================
 *
 * PURPOSE:
 * This header file serves as the master dispatcher for all `php.ini`
 * related functionality within the `quicpro_async` extension.
 *
 * ARCHITECTURE:
 * Instead of declaring all INI variables in a single monolithic file, this
 * header includes the modular `index.h` file from each configuration
 * sub-directory. Each of those index files, in turn, includes the necessary
 * headers for its specific domain (base_layer.h, ini_layer.h, conf_layer.h).
 * This highly modular approach ensures a clean separation of concerns and
 * makes the entire configuration system maintainable and extensible.
 *
 * =========================================================================
 */

#ifndef PHP_QUICPRO_INI_H
#define PHP_QUICPRO_INI_H

#include <php.h>

/*
 * =========================================================================
 * == Modular INI Includes
 * =========================================================================
 * Include the master index file from each configuration module.
 * The corresponding `quicpro_ini.c` will call the registration functions
 * from each module's `index.c`.
 */

#include "app_http3_websockets_webtransport/index.h"
#include "bare_metal_tuning/index.h"
#include "cloud_autoscale/index.h"
#include "cluster_and_process/index.h"
#include "dynamic_admin_api/index.h"
#include "high_perf_compute_and_ai/index.h"
#include "iibin/index.h"
#include "mcp_and_orchestrator/index.h"
#include "native_cdn/index.h"
#include "native_object_store/index.h"
#include "open_telemetry/index.h"
#include "quic_transport/index.h"
#include "router_and_loadbalancer/index.h"
#include "security_and_traffic/index.h"
#include "smart_contracts/index.h"
#include "smart_dns/index.h"
#include "ssh_over_quic/index.h"
#include "state_management/index.h"
#include "tcp_transport/index.h"
#include "tls_and_crypto/index.h"


/*
 * =========================================================================
 * == Public C-API for the Master INI Module
 * =========================================================================
 */

/**
 * @brief The master registration function called during MINIT.
 *
 * This function's implementation in `quicpro_ini.c` is a simple dispatcher
 * that calls the `_register()` function from each configuration sub-module.
 *
 * @param module_number The PHP module number.
 * @return SUCCESS or FAILURE.
 */
int quicpro_ini_register(int module_number);

/**
 * @brief The master un-registration function called during MSHUTDOWN.
 * @return SUCCESS or FAILURE.
 */
int quicpro_ini_unregister(void);


#endif /* PHP_QUICPRO_INI_H */