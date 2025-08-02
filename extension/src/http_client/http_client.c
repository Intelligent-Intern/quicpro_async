/*
 * extension/src/http_client/http_client.c – Comprehensive HTTP Client Implementation
 * =================================================================================
 *
 * Overview
 * --------
 * This file provides a robust and highly configurable HTTP client for PHP userland,
 * leveraging the industry-standard libcurl library. It aims to offer a "perfect"
 * HTTP client by exposing an extensive set of options for HTTP/1.1, HTTP/2,
 * and even preliminary HTTP/3 (when libcurl is built with the necessary backends).
 * This client is designed to be the definitive solution for all outbound TCP-based
 * HTTP communication within the Quicpro ecosystem, serving as a fallback for QUIC
 * where necessary, and as the primary client for traditional HTTP needs.
 *
 * It implements a single, powerful PHP function:
 *
 * • quicpro_http_request_send()
 * – Constructs and sends an HTTP request over a TCP connection using libcurl.
 * It supports various methods, headers, request bodies, and an exhaustive
 * list of configuration options for fine-grained control over the network
 * and protocol behavior.
 *
 * Architectural Philosophy
 * ------------------------
 * The primary goal is to hide the complexity of HTTP protocol nuances (like connection
 * reuse, pipelining, multiplexing, redirects, and intricate TLS settings) behind
 * a simple, yet powerful PHP API. By utilizing libcurl, we gain:
 *
 * 1.  **Robustness and Reliability:** libcurl is a battle-tested library used globally,
 * ensuring high stability and correctness in HTTP/HTTPS communication.
 * 2.  **Protocol Agnosticism (HTTP/1.1, HTTP/2, HTTP/3):** It transparently handles
 * protocol negotiation (via ALPN for HTTP/2 and HTTP/3) and gracefully falls back
 * if a preferred protocol is unavailable. For HTTP/2, it inherently manages
 * stream multiplexing.
 * 3.  **Comprehensive Feature Set:** Access to a vast array of options covering
 * timeouts, redirects, authentication, proxying, cookie handling, custom
 * DNS resolution, client certificates, and more.
 * 4.  **Performance Optimization:** libcurl includes internal optimizations for
 * connection pooling, keep-alive, pipelining (for HTTP/1.1), and multiplexing
 * (for HTTP/2), ensuring efficient resource utilization and low latency.
 *
 * Detailed Functionality
 * ----------------------
 * The `quicpro_http_request_send` function encapsulates the entire HTTP request
 * lifecycle:
 * - **Request Construction:** Takes method, URL, headers, and body.
 * - **Connection Management:** Handles TCP connection establishment, keep-alives,
 * and potential connection reuse.
 * - **Protocol Negotiation:** Automatically or explicitly selects HTTP/1.1, HTTP/2,
 * or HTTP/3 based on configuration and server capabilities.
 * - **TLS Handshake:** Manages client-side TLS handshake, including certificate
 * verification, custom CA bundles, and client certificate authentication.
 * - **Data Transfer:** Efficiently sends request body and receives response
 * headers and body.
 * - **Redirection Handling:** Configurable automatic following of HTTP redirects.
 * - **Error Handling:** Maps libcurl's extensive error codes to meaningful PHP
 * exceptions for robust error management in userland.
 *
 * Dependencies
 * ------------
 * • php_quicpro.h          – Core extension definitions and resource APIs.
 * • http_client.h          – Header for this client's public PHP function declarations.
 * • cancel.h               – Contains error helpers like `throw_mcp_error_as_php_exception()`
 * for consistent exception handling across the extension.
 * • curl/curl.h            – The main header for the libcurl library.
 * • zend_exceptions.h      – PHP API for throwing exceptions.
 * • zend_smart_str.h       – PHP's internal utility for dynamic string building.
 * • ext/standard/url.h     – For potential future URL manipulation if needed (e.g., parsing parts).
 */

#include "php_quicpro.h"
#include "http_client.h"
#include "cancel.h"

#include <curl/curl.h>
#include <zend_exceptions.h>
#include <zend_smart_str.h>
#include <ext/standard/url.h> // Potentially for future advanced URL handling
#include <ctype.h>            // For tolower in header parsing


/**
 * @brief Structure to hold data passed to libcurl's header processing callback.
 *
 * This structure is essential for the `header_callback` function to correctly
 * accumulate all incoming HTTP response headers. The `headers_buf` will store
 * the raw header lines (e.g., "Content-Type: application/json\r\n") before
 * they are parsed into an associative array for PHP userland. The `first_line_parsed`
 * flag ensures that the initial HTTP status line (e.g., "HTTP/1.1 200 OK")
 * is correctly identified and ignored, as it's not a true header.
 */
typedef struct {
    smart_str *headers_buf;      /**< Pointer to a smart_str buffer to append raw header lines. */
    bool first_line_parsed;      /**< Flag: true after the HTTP status line has been encountered and skipped. */
} header_data_t;

/**
 * @brief Callback function for libcurl to write received response body data.
 *
 * This function is registered with `CURLOPT_WRITEFUNCTION` and is invoked by libcurl
 * whenever a chunk of the response body is received from the server. Its purpose
 * is to efficiently append these incoming data chunks to a dynamically-sized
 * `smart_str` buffer, which will eventually contain the complete response body.
 * This approach avoids fixed-size buffer overflows and minimizes memory reallocations.
 *
 * @param contents Pointer to the beginning of the received data chunk.
 * @param size The size of each element in `contents` (typically 1 byte).
 * @param nmemb The number of elements in `contents`.
 * @param userp A generic pointer to the `smart_str` buffer (`response_body`) where data should be written.
 * @return The total number of bytes successfully processed (`size * nmemb`). libcurl expects this value
 * to match `size * nmemb`; a mismatch would indicate an error and abort the transfer.
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    smart_str *buf = (smart_str *)userp;
    size_t realsize = size * nmemb;
    smart_str_appendl(buf, (char *)contents, realsize);
    return realsize;
}

/**
 * @brief Callback function for libcurl to process received response headers.
 *
 * This function is registered with `CURLOPT_HEADERFUNCTION` and is called by libcurl
 * for each line of the HTTP response header section. It intelligently distinguishes
 * between the initial HTTP status line and actual header fields.
 * - The *first line* (e.g., "HTTP/1.1 200 OK") is detected by checking for "HTTP/"
 * and is skipped, as it's handled by `CURLINFO_RESPONSE_CODE`.
 * - *Empty lines* (CRLF) signifying the end of the header section are also skipped.
 * - All *actual header lines* are appended raw to a `smart_str` buffer. This raw
 * collection is later parsed into a structured PHP array.
 * This callback is crucial for capturing all response metadata sent by the server.
 *
 * @param buffer Pointer to the null-terminated header line string.
 * @param size The size of each data element (always 1 for strings).
 * @param nitems The number of data elements (length of the string).
 * @param userp A generic pointer to a `header_data_t` structure, containing the
 * target `smart_str` buffer and a state flag.
 * @return The total number of bytes successfully processed (`size * nitems`).
 * Returning a different value would signal an error to libcurl.
 */
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userp) {
    header_data_t *header_data = (header_data_t *)userp;
    size_t realsize = size * nitems;

    // Detect and skip the HTTP status line (e.g., "HTTP/1.1 200 OK")
    if (!header_data->first_line_parsed) {
        if (realsize > 0 && strncmp(buffer, "HTTP/", 5) == 0) {
            header_data->first_line_parsed = true;
        }
        return realsize; // Always consume the status line even if not parsed as a header
    }

    // Detect and skip empty lines (CRLF indicating end of headers)
    if (realsize == 0 || (realsize == 2 && buffer[0] == '\r' && buffer[1] == '\n')) {
        return realsize;
    }

    // Append the raw header line to the buffer for later detailed parsing
    smart_str_appendl(header_data->headers_buf, buffer, realsize);
    return realsize;
}

/**
 * @brief Parses a raw string of HTTP headers into a PHP associative array.
 *
 * This function takes a raw, concatenated string of header lines (as collected
 * by `header_callback`) and tokenizes it into individual header name-value pairs.
 * It handles:
 * - Newline (`\r\n`) as a delimiter for individual header lines.
 * - Colon (`:`) as a separator between header name and value.
 * - Leading whitespace after the colon.
 * - Normalization of header names to lowercase for consistent array keys.
 * - Concatenation of multiple values for the same header (e.g., multiple `Set-Cookie`
 * headers) into a comma-separated string, mirroring PHP's default behavior.
 *
 * @param raw_headers_str The raw string containing all HTTP headers.
 * @param headers_assoc_array A pointer to a zval (initialized as an array) where
 * the parsed headers will be stored.
 */
static void parse_raw_headers_to_assoc_array(zend_string *raw_headers_str, zval *headers_assoc_array) {
    if (!raw_headers_str || ZSTR_LEN(raw_headers_str) == 0) {
        return;
    }

    char *header_line_start = ZSTR_VAL(raw_headers_str);
    char *header_line_end;
    char *header_name_raw;
    size_t header_name_raw_len;
    char *header_value_raw;
    size_t header_value_raw_len;

    // Iterate through the raw header string, looking for CRLF as line endings
    while ((header_line_end = strstr(header_line_start, "\r\n")) != NULL) {
        // If a blank line is found, it marks the end of headers
        if (header_line_end == header_line_start) {
            header_line_start += 2; // Move past the blank line
            break;
        }

        char *colon = strstr(header_line_start, ":");
        // Ensure a colon exists and is within the current header line
        if (colon != NULL && colon < header_line_end) {
            header_name_raw = header_line_start;
            header_name_raw_len = colon - header_line_start;

            header_value_raw = colon + 1;
            // Skip any leading whitespace or tabs after the colon
            while (*header_value_raw == ' ' || *header_value_raw == '\t') {
                header_value_raw++;
            }
            header_value_raw_len = (header_line_end - header_value_raw);
            
            // Normalize header name to lowercase for consistent array keys in PHP
            zend_string *normalized_name = zend_string_alloc(header_name_raw_len, 0);
            for (size_t i = 0; i < header_name_raw_len; i++) {
                ZSTR_VAL(normalized_name)[i] = tolower((unsigned char)header_name_raw[i]);
            }
            ZSTR_VAL(normalized_name)[header_name_raw_len] = '\0'; // Null-terminate the new string

            // Check if this header name already exists in the associative array
            zval *existing_header_val = zend_hash_find(Z_ARRVAL_P(headers_assoc_array), normalized_name);
            if (existing_header_val && Z_TYPE_P(existing_header_val) == IS_STRING) {
                // If it exists, concatenate the new value with the old, separated by ", "
                zend_string *old_val = Z_STR_P(existing_header_val);
                zend_string *new_concat_val = zend_string_alloc(ZSTR_LEN(old_val) + 2 + header_value_raw_len, 0);
                memcpy(ZSTR_VAL(new_concat_val), ZSTR_VAL(old_val), ZSTR_LEN(old_val));
                memcpy(ZSTR_VAL(new_concat_val) + ZSTR_LEN(old_val), ", ", 2);
                memcpy(ZSTR_VAL(new_concat_val) + ZSTR_LEN(old_val) + 2, header_value_raw, header_value_raw_len);
                ZSTR_VAL(new_concat_val)[ZSTR_LEN(old_val) + 2 + header_value_raw_len] = '\0';
                ZVAL_STR(existing_header_val, new_concat_val); // Update the zval in the hash table
                zend_string_release(old_val); // Release the reference to the old zend_string
            } else {
                // Otherwise, add the new header name and value to the array
                add_assoc_stringl_ex(headers_assoc_array, ZSTR_VAL(normalized_name), ZSTR_LEN(normalized_name), header_value_raw, header_value_raw_len);
            }
            zend_string_release(normalized_name); // Release the temporary zend_string for the normalized name
        }
        header_line_start = header_line_end + 2; // Move to the start of the next line
    }
}


/**
 * @brief Sends a full-featured HTTP request using libcurl.
 *
 * This function serves as the primary entry point for sending HTTP requests
 * from PHP userland through the Quicpro extension's robust HTTP client.
 * It supports a wide range of HTTP methods, custom headers, request bodies,
 * and comprehensive configuration options.
 *
 * The function encapsulates the entire logic of setting up a libcurl easy handle,
 * configuring it with various options (timeouts, redirects, SSL/TLS, network),
 * executing the request, and then processing the response (status, body, headers).
 *
 * @param ZEND_EXEC_ARGS Standard PHP function arguments.
 * @return A PHP array on success, containing:
 * - 'status' (int): The HTTP response status code.
 * - 'body' (string): The complete HTTP response body.
 * - 'headers' (array): An associative array of normalized HTTP response headers.
 * Returns FALSE on failure, automatically throwing a `Quicpro\Exception\McpException`
 * with a detailed error message from libcurl.
 */
PHP_FUNCTION(quicpro_http_request_send) {
    // PHP function parameters:
    char *url_str;                   // The target URL for the request
    size_t url_len;
    char *method_str = "GET";        // HTTP method (GET, POST, PUT, etc.), defaults to GET
    size_t method_len = 3;
    zval *headers_array = NULL;      // Associative array of request headers
    char *body_str = NULL;           // Request body string
    size_t body_len = 0;
    zval *options_array = NULL;      // Associative array of additional cURL options

    // Parse incoming PHP parameters
    ZEND_PARSE_PARAMETERS_START(1, 5)
        Z_PARAM_STRING(url_str, url_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(method_str, method_len)
        Z_PARAM_ARRAY_OR_NULL(headers_array)
        Z_PARAM_STRING_OR_NULL(body_str, body_len)
        Z_PARAM_ARRAY_OR_NULL(options_array)
    ZEND_PARSE_PARAMETERS_END();

    // Internal libcurl and data handling variables
    CURL *curl;                        // libcurl easy handle
    CURLcode res;                      // Result code from cURL operations
    smart_str response_body = {0};     // Smart string buffer for the response body
    smart_str response_headers_raw = {0}; // Smart string buffer for raw response headers
    struct curl_slist *headers_list = NULL; // Linked list for custom request headers (libcurl format)
    long http_code = 0;                // HTTP response status code
    header_data_t header_data = { .headers_buf = &response_headers_raw, .first_line_parsed = false }; // Data for header callback

    // Initialize the libcurl easy handle. This is the first critical step.
    curl = curl_easy_init();
    if (!curl) {
        throw_mcp_error_as_php_exception(0, "Failed to initialize cURL handle for HTTP request.");
        RETURN_FALSE;
    }

    // --- Fundamental cURL Options Configuration ---
    // Set the target URL for the request.
    curl_easy_setopt(curl, CURLOPT_URL, url_str);
    // Set the HTTP method (e.g., "GET", "POST", "PUT").
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method_str);
    // Register the callback function for writing the response body data.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    // Pass the smart_str buffer to the write callback.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_body);
    // Register the callback function for processing response headers.
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    // Pass the header_data_t structure to the header callback.
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&header_data);

    // --- TCP Keep-Alive Configuration (Essential for persistent connections) ---
    // Enable TCP Keep-Alive probes to detect dead connections and free resources.
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    // Set the idle time before sending the first keep-alive probe (in seconds).
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 60L);
    // Set the interval between subsequent keep-alive probes (in seconds).
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 30L);
    // Ensure that libcurl reuses connections for subsequent requests where possible.
    // This is implicitly handled by libcurl's connection pooling but explicitly stated here for clarity.
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 0L); // Allow reuse


    // --- Process Request Headers from PHP Array ---
    // Iterate over the associative array of headers provided from PHP userland.
    if (headers_array && Z_TYPE_P(headers_array) == IS_ARRAY) {
        zend_string *key; // Header name
        zval *val;        // Header value
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(headers_array), key, val) {
            // Validate header key and value types. Only string keys and string values are valid for headers.
            if (!key || Z_TYPE_P(val) != IS_STRING) {
                // For a perfect client, we should probably throw an exception here,
                // but for robustness, we'll silently skip invalid headers.
                continue;
            }
            smart_str header_line = {0};
            smart_str_appendl(&header_line, ZSTR_VAL(key), ZSTR_LEN(key)); // Append header name
            smart_str_appendl(&header_line, ": ", 2);                     // Append ": " separator
            smart_str_appendl(&header_line, Z_STRVAL_P(val), Z_STRLEN_P(val)); // Append header value
            smart_str_0(&header_line); // Null-terminate the string for `curl_slist_append`
            // Add the formatted header string to libcurl's internal linked list for headers.
            headers_list = curl_slist_append(headers_list, ZSTR_VAL(header_line.s));
            smart_str_free(&header_line); // Free the temporary zend_string allocated by smart_str_0
        } ZEND_HASH_FOREACH_END();
        // If any headers were successfully added, set them in the cURL handle.
        if (headers_list) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
        }
    }

    // --- Set Request Body for Appropriate HTTP Methods ---
    // If a request body is provided and the method implies a body (POST, PUT, PATCH),
    // configure libcurl to send it.
    if (body_str && body_len > 0 && (strcmp(method_str, "POST") == 0 || strcmp(method_str, "PUT") == 0 || strcmp(method_str, "PATCH") == 0)) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str);    // The data to send
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_len); // The size of the data
    } else if (body_str && body_len > 0) {
        // If a body is provided for methods like GET/HEAD, it's typically ignored by servers.
        // For strict HTTP compliance, we might warn or error here. For now, libcurl will handle.
    }


    // --- Process Extensive Options Provided by PHP Userland ---
    // This section allows granular control over cURL behavior via an associative PHP array.
    if (options_array && Z_TYPE_P(options_array) == IS_ARRAY) {
        zval *opt_val; // Pointer to the value of an option in the PHP array

        // Network Timeouts
        // CURLOPT_TIMEOUT_MS: Maximum time in milliseconds that the request is allowed to take.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "timeout_ms", sizeof("timeout_ms") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, Z_LVAL_P(opt_val));
        }
        // CURLOPT_CONNECTTIMEOUT_MS: Maximum time in milliseconds that the connection phase is allowed to take.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "connect_timeout_ms", sizeof("connect_timeout_ms") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, Z_LVAL_P(opt_val));
        }

        // HTTP Redirect Handling
        // CURLOPT_FOLLOWLOCATION: Instructs libcurl to follow HTTP 3xx redirects.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "follow_redirects", sizeof("follow_redirects") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        }
        // CURLOPT_MAXREDIRS: Maximum number of redirects to follow.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "max_redirects", sizeof("max_redirects") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, Z_LVAL_P(opt_val));
        }

        // HTTP Version Preference (ALPN negotiation is handled automatically by libcurl)
        // Allows forcing a specific HTTP version or letting libcurl negotiate.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "http_version", sizeof("http_version") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            if (zend_string_equals_literal(Z_STR_P(opt_val), "2.0")) {
                curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2_0);
                // CURLOPT_PIPEWAIT: For HTTP/2, wait for a multiplexed connection to become available.
                // This is crucial for efficient connection reuse with HTTP/2.
                curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);
            } else if (zend_string_equals_literal(Z_STR_P(opt_val), "1.1")) {
                curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_1);
            } else if (zend_string_equals_literal(Z_STR_P(opt_val), "3.0")) {
                // When libcurl is compiled with nghttp3/quiche support, this enables HTTP/3.
                // This option signifies the intent to use HTTP/3 over UDP.
                curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_3);
            } else {
                // If an invalid or unspecified version is given, allow libcurl to pick the best.
                curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_NONE);
            }
        }

        // Verbose Debugging Output
        // CURLOPT_VERBOSE: Enables verbose output from libcurl to stderr, useful for debugging network issues.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "verbose", sizeof("verbose") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        }

        // Network Socket Options
        // CURLOPT_TCP_NODELAY: Disables the Nagle algorithm for TCP, reducing latency at the cost of some bandwidth efficiency.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "tcp_nodelay", sizeof("tcp_nodelay") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
        }
        // CURLOPT_INTERFACE: Specifies the outbound network interface to use. Useful in multi-homed environments.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "interface", sizeof("interface") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_INTERFACE, Z_STRVAL_P(opt_val));
        }

        // TLS/SSL Configuration (Comprehensive client-side TLS control)
        // CURLOPT_SSL_VERIFYPEER: Enables/disables peer certificate verification. Set to 0L to disable (NOT RECOMMENDED for production).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "verify_peer", sizeof("verify_peer") - 1)) != NULL && zend_is_false(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        // CURLOPT_SSL_VERIFYHOST: Controls hostname verification in the peer's certificate. Set to 0L to disable (NOT RECOMMENDED).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "verify_host", sizeof("verify_host") - 1)) != NULL && zend_is_false(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 0 = don't verify, 1 = verify CA, 2 = verify CA & hostname. 0 is for "false".
        }
        // CURLOPT_CAINFO: Path to a file containing a concatenated list of CA certificates for peer verification.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "ca_info", sizeof("ca_info") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_SSLCERT: Path to the client's public key certificate file for mutual TLS.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "cert_file", sizeof("cert_file") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_SSLCERT, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_SSLKEY: Path to the client's private key file associated with CURLOPT_SSLCERT.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "key_file", sizeof("key_file") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_SSLKEY, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_KEYPASSWD: Password for the private key file (if encrypted).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "key_passwd", sizeof("key_passwd") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_KEYPASSWD, Z_STRVAL_P(opt_val));
        }

        // DNS Resolution Options (for advanced scenarios like custom DNS or host pinning)
        // CURLOPT_RESOLVE: Allows specifying a custom IP address for a hostname:port combination, bypassing DNS lookup.
        // Format: "hostname:port:IPaddress" e.g., "example.com:80:192.168.0.1"
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "resolve_host", sizeof("resolve_host") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            struct curl_slist *resolve_list = NULL;
            resolve_list = curl_slist_append(resolve_list, Z_STRVAL_P(opt_val));
            curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
            // Note: `resolve_list` is owned by libcurl after this call and will be freed by `curl_easy_cleanup`.
            // If multiple `resolve_host` entries are needed, this option needs to be extended to support a PHP array of strings.
        }
        // CURLOPT_DNS_CACHE_TIMEOUT: Sets the DNS cache timeout in seconds. A value of 0 means forever.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "dns_cache_timeout", sizeof("dns_cache_timeout") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, Z_LVAL_P(opt_val));
        }
        // CURLOPT_DNS_USE_GLOBAL_CACHE: Enables/disables use of the global DNS cache.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "dns_use_global_cache", sizeof("dns_use_global_cache") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 1L);
        }
        // CURLOPT_DNS_SERVERS: Comma-separated list of DNS servers to use.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "dns_servers", sizeof("dns_servers") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_DNS_SERVERS, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_IPRESOLVE: Force resolving to IPv4, IPv6 or leave it to libcurl (default).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "ip_resolve", sizeof("ip_resolve") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            if (zend_string_equals_literal(Z_STR_P(opt_val), "ipv4")) {
                curl_easy_setopt(curl, CURLOPT_IPRESOLVE, (long)CURL_IPRESOLVE_V4);
            } else if (zend_string_equals_literal(Z_STR_P(opt_val), "ipv6")) {
                curl_easy_setopt(curl, CURLOPT_IPRESOLVE, (long)CURL_IPRESOLVE_V6);
            } else { // auto
                curl_easy_setopt(curl, CURLOPT_IPRESOLVE, (long)CURL_IPRESOLVE_WHATEVER);
            }
        }
        // CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS: Timeout for Happy Eyeballs connection attempts.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "happy_eyeballs_timeout_ms", sizeof("happy_eyeballs_timeout_ms") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, Z_LVAL_P(opt_val));
        }

        // Proxy Settings
        // CURLOPT_PROXY: The HTTP/HTTPS proxy to use.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "proxy", sizeof("proxy") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_PROXY, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_PROXYUSERPWD: User and password for proxy authentication. Format: "user:password".
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "proxy_userpwd", sizeof("proxy_userpwd") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_PROXYTYPE: Type of proxy (HTTP, SOCKS4, SOCKS5).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "proxy_type", sizeof("proxy_type") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            if (zend_string_equals_literal(Z_STR_P(opt_val), "http")) {
                curl_easy_setopt(curl, CURLOPT_PROXYTYPE, (long)CURLPROXY_HTTP);
            } else if (zend_string_equals_literal(Z_STR_P(opt_val), "socks4")) {
                curl_easy_setopt(curl, CURLOPT_PROXYTYPE, (long)CURLPROXY_SOCKS4);
            } else if (zend_string_equals_literal(Z_STR_P(opt_val), "socks5")) {
                curl_easy_setopt(curl, CURLOPT_PROXYTYPE, (long)CURLPROXY_SOCKS5);
            }
        }
        // CURLOPT_NOPROXY: Comma-separated list of hosts that do not use the proxy.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "no_proxy", sizeof("no_proxy") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_NOPROXY, Z_STRVAL_P(opt_val));
        }

        // Authentication Settings
        // CURLOPT_HTTPAUTH: HTTP authentication method(s) to use.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "http_auth", sizeof("http_auth") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, Z_LVAL_P(opt_val)); // Expects CURLAUTH_BASIC, CURLAUTH_DIGEST etc.
        }
        // CURLOPT_USERPWD: User and password for HTTP authentication. Format: "user:password".
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "user_pwd", sizeof("user_pwd") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_USERPWD, Z_STRVAL_P(opt_val));
        }

        // Additional transfer options
        // CURLOPT_FRESH_CONNECT: Forces a new connection to be used instead of a cached one.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "fresh_connect", sizeof("fresh_connect") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);
        }
        // CURLOPT_DNS_USE_GLOBAL_CACHE: Whether to use the global DNS cache.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "dns_use_global_cache", sizeof("dns_use_global_cache") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 1L);
        }
        // CURLOPT_MAXFILESIZE: Maximum file size to download.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "max_file_size", sizeof("max_file_size") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, Z_LVAL_P(opt_val));
        }
        // CURLOPT_ACCEPT_ENCODING: Request specific content encodings.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "accept_encoding", sizeof("accept_encoding") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_COOKIE: Send a specific cookie string.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "cookie", sizeof("cookie") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_COOKIE, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_COOKIEFILE: Read cookies from this file.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "cookie_file", sizeof("cookie_file") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_COOKIEFILE, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_COOKIEJAR: Write cookies to this file after the request.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "cookie_jar", sizeof("cookie_jar") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_REFERER: Set the Referer header.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "referer", sizeof("referer") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_REFERER, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_USERAGENT: Set the User-Agent header.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "user_agent", sizeof("user_agent") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_CRLF: Convert Unix newlines to CRLF newlines (important for some FTP servers).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "crlf", sizeof("crlf") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_CRLF, 1L);
        }
        // CURLOPT_BUFFERSIZE: Preferred receive buffer size.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "buffer_size", sizeof("buffer_size") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, Z_LVAL_P(opt_val));
        }
        // CURLOPT_HTTPPROXYTUNNEL: Tunnel through HTTP proxy.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "http_proxy_tunnel", sizeof("http_proxy_tunnel") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1L);
        }
        // CURLOPT_TRANSFERTEXT: Treat received data as text (perform newline conversions).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "transfer_text", sizeof("transfer_text") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_TRANSFERTEXT, 1L);
        }
        // CURLOPT_UNRESTRICTED_AUTH: Send authentication to all hosts, not just the original one.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "unrestricted_auth", sizeof("unrestricted_auth") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);
        }
        // CURLOPT_PUT: Set PUT method (alternative to CUSTOMREQUEST for PUT).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "put", sizeof("put") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        }
        // CURLOPT_POST: Set POST method (alternative to CUSTOMREQUEST for POST).
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "post", sizeof("post") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }
        // CURLOPT_SSL_CIPHER_LIST: List of ciphers to use for TLS.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "ssl_cipher_list", sizeof("ssl_cipher_list") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, Z_STRVAL_P(opt_val));
        }
        // CURLOPT_SSL_MAXCONN: Max number of SSL connections to cache.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "ssl_max_conn", sizeof("ssl_max_conn") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_SSL_MAXCONN, Z_LVAL_P(opt_val));
        }
        // CURLOPT_SUPPRESS_CONNECT_HEADERS: Don't send proxy CONNECT headers.
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "suppress_connect_headers", sizeof("suppress_connect_headers") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_SUPPRESS_CONNECT_HEADERS, 1L);
        }
        // CURLOPT_TCP_FASTOPEN: Enable TCP Fast Open (Linux only).
        #ifdef CURLOPT_TCP_FASTOPEN
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "tcp_fastopen", sizeof("tcp_fastopen") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);
        }
        #endif
        // CURLOPT_ALTSVC: Enable/disable ALTSVC.
        #ifdef CURLOPT_ALTSVC
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "altsvc", sizeof("altsvc") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_ALTSVC, Z_STRVAL_P(opt_val)); // e.g., "clear" or "h3"
        }
        #endif
        // CURLOPT_DNS_LOCAL_IP4/6: Specify local IPv4/6 address for DNS lookup.
        #ifdef CURLOPT_DNS_LOCAL_IP4
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "dns_local_ip4", sizeof("dns_local_ip4") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_DNS_LOCAL_IP4, Z_STRVAL_P(opt_val));
        }
        #endif
        #ifdef CURLOPT_DNS_LOCAL_IP6
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "dns_local_ip6", sizeof("dns_local_ip6") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_STRING) {
            curl_easy_setopt(curl, CURLOPT_DNS_LOCAL_IP6, Z_STRVAL_P(opt_val));
        }
        #endif
        // CURLOPT_KEEP_SENDING_ON_ERROR: Continue sending data on error.
        #ifdef CURLOPT_KEEP_SENDING_ON_ERROR
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "keep_sending_on_error", sizeof("keep_sending_on_error") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_KEEP_SENDING_ON_ERROR, 1L);
        }
        #endif
        // CURLOPT_SSH_COMPRESSION: Enable SSH compression (for SCP/SFTP protocols).
        #ifdef CURLOPT_SSH_COMPRESSION
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "ssh_compression", sizeof("ssh_compression") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_SSH_COMPRESSION, 1L);
        }
        #endif
        // CURLOPT_SSL_OPTIONS: Various SSL options.
        #ifdef CURLOPT_SSL_OPTIONS
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "ssl_options", sizeof("ssl_options") - 1)) != NULL && Z_TYPE_P(opt_val) == IS_LONG) {
            curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, Z_LVAL_P(opt_val)); // e.g., CURLSSLOPT_NO_REVOKE
        }
        #endif
        // CURLOPT_HTTP09_ALLOWED: Allow HTTP/0.9 responses.
        #ifdef CURLOPT_HTTP09_ALLOWED
        if ((opt_val = zend_hash_str_find(Z_ARRVAL_P(options_array), "http09_allowed", sizeof("http09_allowed") - 1)) != NULL && zend_is_true(opt_val)) {
            curl_easy_setopt(curl, CURLOPT_HTTP09_ALLOWED, 1L);
        }
        #endif
    }

    // --- Execute the cURL request ---
    res = curl_easy_perform(curl);
    // Check for cURL errors
    if (res != CURLE_OK) {
        throw_mcp_error_as_php_exception(0, "cURL request failed: %s", curl_easy_strerror(res));
        // Free dynamically allocated smart_str buffers and curl_slist before returning
        smart_str_free(&response_body);
        smart_str_free(&response_headers_raw);
        curl_slist_free_all(headers_list);
        curl_easy_cleanup(curl); // Always clean up the curl handle
        RETURN_FALSE;
    }

    // --- Retrieve HTTP Response Status Code ---
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // --- Prepare PHP Return Value Array ---
    array_init(return_value);
    add_assoc_long(return_value, "status", http_code);
    // Add the response body to the return array. If empty, use PHP's empty string.
    add_assoc_str(return_value, "body", response_body.s ? response_body.s : zend_empty_string);

    // --- Parse and Add Response Headers to Return Array ---
    zval headers_assoc_array; // Declare a zval for the associative headers array
    array_init(&headers_assoc_array); // Initialize it as an empty PHP array
    // Only parse if raw headers were actually received
    if (response_headers_raw.s && ZSTR_LEN(response_headers_raw.s) > 0) {
        parse_raw_headers_to_assoc_array(response_headers_raw.s, &headers_assoc_array);
    }
    // Add the parsed headers array to the main return array.
    // Ownership of `headers_assoc_array` is transferred to `return_value`.
    add_assoc_zval(return_value, "headers", &headers_assoc_array);

    // --- Final Cleanup of Resources ---
    smart_str_free(&response_body);          // Free the smart_str allocated for the body
    smart_str_free(&response_headers_raw);   // Free the smart_str allocated for raw headers
    curl_slist_free_all(headers_list);       // Free the linked list of request headers
    curl_easy_cleanup(curl);                 // Clean up the cURL easy handle, releasing all associated resources
}