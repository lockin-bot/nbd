/*
 * VSOCK support for NBD - Implementation
 *
 * This file contains vsock-specific functions for adding vsock support
 * to both nbd-server and nbd-client.
 */

#include "config.h"
#include "vsock_support.h"

#ifdef HAVE_LINUX_VM_SOCKETS_H

#include <netinet/in.h>
#include <arpa/inet.h>

/* Global flag for vsock support detection */
static int vsock_support_checked = 0;
static int vsock_support_available = 0;

/**
 * Check if vsock support is available on this system
 * @return 1 if vsock is supported, 0 otherwise
 */
int is_vsock_supported(void) {
    int sock;

    if (vsock_support_checked) {
        return vsock_support_available;
    }

    vsock_support_checked = 1;

    /* Try to create a vsock socket to test support */
    sock = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (sock >= 0) {
        close(sock);
        vsock_support_available = 1;
    } else {
        vsock_support_available = 0;
    }

    return vsock_support_available;
}

/**
 * Parse a CID string into an unsigned integer
 * @param cid_str: String containing the CID
 * @return CID value, or 0 on error
 */
unsigned int parse_vsock_cid(const char *cid_str) {
    char *endptr;
    unsigned long cid;

    if (!cid_str || *cid_str == '\0') {
        return 0;
    }

    /* Handle special CID names */
    if (strcmp(cid_str, "hypervisor") == 0) {
        return VMADDR_CID_HYPERVISOR;
    } else if (strcmp(cid_str, "host") == 0) {
        return VMADDR_CID_HOST;
    } else if (strcmp(cid_str, "local") == 0) {
        return VMADDR_CID_LOCAL;
    } else if (strcmp(cid_str, "any") == 0) {
        return VMADDR_CID_ANY;
    }

    /* Parse numeric CID */
    errno = 0;
    cid = strtoul(cid_str, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || cid > 0xFFFFFFFF) {
        return 0;
    }

    return (unsigned int)cid;
}

/**
 * Parse a port string into an unsigned integer
 * @param port_str: String containing the port
 * @return Port value, or 0 on error
 */
unsigned int parse_vsock_port(const char *port_str) {
    char *endptr;
    unsigned long port;

    if (!port_str || *port_str == '\0') {
        return 0;
    }

    errno = 0;
    port = strtoul(port_str, &endptr, 10);

    if (errno != 0 || *endptr != '\0' || port > 0xFFFFFFFF) {
        return 0;
    }

    return (unsigned int)port;
}

/**
 * Convert a CID to a string representation
 * @param cid: CID value
 * @param buf: Buffer to store the string
 * @param buf_size: Size of the buffer
 * @return Pointer to buf, or NULL on error
 */
const char *cid_to_string(unsigned int cid, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) {
        return NULL;
    }

    switch (cid) {
        case VMADDR_CID_HYPERVISOR:
            snprintf(buf, buf_size, "hypervisor");
            break;
        case VMADDR_CID_HOST:
            snprintf(buf, buf_size, "host");
            break;
        case VMADDR_CID_LOCAL:
            snprintf(buf, buf_size, "local");
            break;
        case VMADDR_CID_ANY:
            snprintf(buf, buf_size, "any");
            break;
        default:
            snprintf(buf, buf_size, "%u", cid);
            break;
    }

    return buf;
}

/**
 * Convert a vsock address to a string representation
 * @param addr: Vsock address structure
 * @param buf: Buffer to store the string
 * @param buf_size: Size of the buffer
 * @return Pointer to buf, or NULL on error
 */
int vsock_address_to_string(const struct sockaddr_vm *addr, char *buf, size_t buf_size) {
    char cid_str[32];

    if (!addr || !buf || buf_size == 0 || addr->svm_family != AF_VSOCK) {
        return -1;
    }

    if (!cid_to_string(addr->svm_cid, cid_str, sizeof(cid_str))) {
        return -1;
    }

    snprintf(buf, buf_size, "vsock://%s:%u", cid_str, addr->svm_port);

    return 0;
}

/**
 * Create a vsock socket
 * @return Socket file descriptor, or -1 on error
 */
int create_vsock_socket(void) {
    int sock;

    sock = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    return sock;
}

/**
 * Bind a vsock socket to a CID and port
 * @param sock: Socket file descriptor
 * @param cid: CID to bind to
 * @param port: Port to bind to
 * @return 0 on success, -1 on error
 */
int bind_vsock_socket(int sock, unsigned int cid, unsigned int port) {
    struct sockaddr_vm addr;

    if (sock < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.svm_family = AF_VSOCK;
    addr.svm_cid = cid;
    addr.svm_port = port;
    addr.svm_flags = 0;

    return bind(sock, (struct sockaddr *)&addr, sizeof(addr));
}

/**
 * Listen for connections on a vsock socket
 * @param sock: Socket file descriptor
 * @param backlog: Maximum number of pending connections
 * @return 0 on success, -1 on error
 */
int listen_vsock_socket(int sock, int backlog) {
    if (sock < 0) {
        return -1;
    }

    return listen(sock, backlog);
}

/**
 * Connect to a vsock address
 * @param sock: Socket file descriptor
 * @param cid: Destination CID
 * @param port: Destination port
 * @return 0 on success, -1 on error
 */
int connect_vsock_socket(int sock, unsigned int cid, unsigned int port) {
    struct sockaddr_vm addr;

    if (sock < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.svm_family = AF_VSOCK;
    addr.svm_cid = cid;
    addr.svm_port = port;
    addr.svm_flags = 0;

    return connect(sock, (struct sockaddr *)&addr, sizeof(addr));
}

/**
 * Accept a vsock connection
 * @param listen_sock: Listening socket file descriptor
 * @param client_addr: Buffer to store client address (can be NULL)
 * @param addr_len: Size of address buffer (can be NULL)
 * @return New socket file descriptor, or -1 on error
 */
int accept_vsock_connection(int listen_sock, struct sockaddr_vm *client_addr, socklen_t *addr_len) {
    struct sockaddr_vm addr;
    socklen_t len = sizeof(addr);

    if (listen_sock < 0) {
        return -1;
    }

    if (!client_addr) {
        client_addr = &addr;
    }
    if (!addr_len) {
        addr_len = &len;
    }

    return accept(listen_sock, (struct sockaddr *)client_addr, addr_len);
}

/**
 * Set vsock socket buffer size
 * @param sock: Socket file descriptor
 * @param size: Buffer size
 * @return 0 on success, -1 on error
 */
int set_vsock_buffer_size(int sock, unsigned long long size) {
    if (sock < 0) {
        return -1;
    }

    return setsockopt(sock, AF_VSOCK, SO_VM_SOCKETS_BUFFER_SIZE, &size, sizeof(size));
}

/**
 * Set vsock socket connection timeout
 * @param sock: Socket file descriptor
 * @param timeout_sec: Timeout in seconds
 * @return 0 on success, -1 on error
 */
int set_vsock_connect_timeout(int sock, unsigned int timeout_sec) {
#ifdef SO_VM_SOCKETS_CONNECT_TIMEOUT
    if (sock < 0) {
        return -1;
    }

    return setsockopt(sock, AF_VSOCK, SO_VM_SOCKETS_CONNECT_TIMEOUT, &timeout_sec, sizeof(timeout_sec));
#else
    return 0; /* Not supported, but don't fail */
#endif
}

/**
 * Get the peer CID from a vsock socket
 * @param sock: Socket file descriptor
 * @param cid: Pointer to store the peer CID
 * @return 0 on success, -1 on error
 */
int get_vsock_peer_cid(int sock, unsigned int *cid) {
#ifdef SO_VM_SOCKETS_PEER_HOST_VM_ID
    int peer_cid;
    socklen_t len = sizeof(peer_cid);

    if (sock < 0 || !cid) {
        return -1;
    }

    if (getsockopt(sock, AF_VSOCK, SO_VM_SOCKETS_PEER_HOST_VM_ID, &peer_cid, &len) < 0) {
        return -1;
    }

    *cid = (unsigned int)peer_cid;
    return 0;
#else
    return -1; /* Not supported */
#endif
}

/**
 * Parse a vsock address string (e.g., "vsock://2:10809" or "2:10809")
 * @param addr_str: Address string to parse
 * @param cid: Pointer to store the parsed CID
 * @param port: Pointer to store the parsed port
 * @return 0 on success, -1 on error
 */
int parse_vsock_address(const char *addr_str, unsigned int *cid, unsigned int *port) {
    const char *cid_start, *port_start;
    char *cid_str, *port_str;
    int ret = -1;

    if (!addr_str || !cid || !port) {
        return -1;
    }

    /* Skip "vsock://" prefix if present */
    if (strncmp(addr_str, "vsock://", 8) == 0) {
        cid_start = addr_str + 8;
    } else {
        cid_start = addr_str;
    }

    /* Find the colon separator */
    port_start = strchr(cid_start, ':');
    if (!port_start) {
        return -1;
    }

    /* Extract CID and port strings */
    cid_str = strndup(cid_start, port_start - cid_start);
    port_str = strdup(port_start + 1);

    if (!cid_str || !port_str) {
        goto cleanup;
    }

    /* Parse CID and port */
    *cid = parse_vsock_cid(cid_str);
    *port = parse_vsock_port(port_str);

    if (*cid != 0 && *port != 0 && is_valid_cid(*cid) && is_valid_port(*port)) {
        ret = 0;
    }

cleanup:
    if (cid_str) free(cid_str);
    if (port_str) free(port_str);

    return ret;
}

/**
 * Check if a CID is valid
 * @param cid: CID to check
 * @return 1 if valid, 0 otherwise
 */
int is_valid_cid(unsigned int cid) {
    /* All CIDs are technically valid, but we can add specific validation here if needed */
    return 1;
}

/**
 * Check if a port is valid
 * @param port: Port to check
 * @return 1 if valid, 0 otherwise
 */
int is_valid_port(unsigned int port) {
    return (port >= VSOCK_MIN_PORT && port <= VSOCK_MAX_PORT) || port == VMADDR_PORT_ANY;
}

/**
 * Initialize vsock configuration structure
 * @param config: Configuration structure to initialize
 * @return 0 on success, -1 on error
 */
int init_vsock_config(struct vsock_config *config) {
    if (!config) {
        return -1;
    }

    memset(config, 0, sizeof(*config));
    config->cid = VMADDR_CID_ANY;
    config->port = NBD_DEFAULT_VSOCK_PORT;
    config->enabled = 0;

    return 0;
}

/**
 * Clean up vsock configuration structure
 * @param config: Configuration structure to clean up
 */
void cleanup_vsock_config(struct vsock_config *config) {
    if (!config) {
        return;
    }

    if (config->listen_address) {
        free(config->listen_address);
        config->listen_address = NULL;
    }

    if (config->connect_address) {
        free(config->connect_address);
        config->connect_address = NULL;
    }
}

/**
 * Parse vsock configuration option
 * @param key: Configuration key
 * @param value: Configuration value
 * @param config: Configuration structure to update
 * @return 0 on success, -1 on error
 */
int parse_vsock_config_option(const char *key, const char *value, struct vsock_config *config) {
    if (!key || !value || !config) {
        return -1;
    }

    if (strcmp(key, "vsockcid") == 0) {
        config->cid = parse_vsock_cid(value);
        config->enabled = 1;
    } else if (strcmp(key, "vsockport") == 0) {
        config->port = parse_vsock_port(value);
        config->enabled = 1;
    } else if (strcmp(key, "vsocklisten") == 0) {
        config->enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else {
        return -1;
    }

    return 0;
}

/**
 * Print vsock error message
 * @param prefix: Prefix string
 * @param error_code: Error code (usually errno)
 */
void vsock_perror(const char *prefix, int error_code) {
    if (prefix) {
        fprintf(stderr, "%s: %s (errno=%d)\n", prefix, strerror(error_code), error_code);
    } else {
        fprintf(stderr, "VSOCK error: %s (errno=%d)\n", strerror(error_code), error_code);
    }
}

/**
 * Get vsock-specific error string
 * @param error_code: Error code
 * @return Error string
 */
const char *vsock_strerror(int error_code) {
    return strerror(error_code);
}

#endif /* HAVE_LINUX_VM_SOCKETS_H */