#include "php_quicpro.h"
PHP_FUNCTION(quicpro_export_session_ticket){
    // stub
    RETURN_EMPTY_STRING();
}
PHP_FUNCTION(quicpro_import_session_ticket){
    RETURN_FALSE;
}
PHP_FUNCTION(quicpro_set_ca_file){
    RETURN_FALSE;
}
PHP_FUNCTION(quicpro_set_client_cert){
    RETURN_FALSE;
}
PHP_FUNCTION(quicpro_get_last_error){
    RETURN_STRING("");
}
PHP_FUNCTION(quicpro_get_stats){
    RETURN_EMPTY_ARRAY;
}
PHP_FUNCTION(quicpro_version){
    RETURN_STRING("0.1.0");
}
