/*
 * VSOCK support for NBD - Header file
 *
 * This file contains vsock-specific structures, constants, and function
 * prototypes for adding vsock support to both nbd-server and nbd-client.
 */

#ifndef VSOCK_SUPPORT_H
#define VSOCK_SUPPORT_H

#include "config.h"

#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

#include "cliserv.h"

#ifdef HAVE_LINUX_VM_SOCKETS_H

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <linux/vm_sockets.h>

/* VSOCK utility functions */
int is_vsock_supported(void);
unsigned int parse_vsock_cid(const char *cid_str);
unsigned int parse_vsock_port(const char *port_str);
const char *cid_to_string(unsigned int cid, char *buf, size_t buf_size);
int vsock_address_to_string(const struct sockaddr_vm *addr, char *buf, size_t buf_size);

/* VSOCK socket creation functions */
int create_vsock_socket(void);
int bind_vsock_socket(int sock, unsigned int cid, unsigned int port);
int listen_vsock_socket(int sock, int backlog);
int connect_vsock_socket(int sock, unsigned int cid, unsigned int port);
int accept_vsock_connection(int listen_sock, struct sockaddr_vm *client_addr, socklen_t *addr_len);

/* VSOCK socket option functions */
int set_vsock_buffer_size(int sock, unsigned long long size);
int set_vsock_connect_timeout(int sock, unsigned int timeout_sec);
int get_vsock_peer_cid(int sock, unsigned int *cid);

/* VSOCK address parsing and validation */
int parse_vsock_address(const char *addr_str, unsigned int *cid, unsigned int *port);
int is_valid_cid(unsigned int cid);
int is_valid_port(unsigned int port);

/* VSOCK configuration support */
struct vsock_config {
    unsigned int cid;
    unsigned int port;
    int enabled;
    char *listen_address;  /* For server: CID to bind to */
    char *connect_address; /* For client: CID to connect to */
};

int init_vsock_config(struct vsock_config *config);
void cleanup_vsock_config(struct vsock_config *config);
int parse_vsock_config_option(const char *key, const char *value, struct vsock_config *config);

/* Error handling */
void vsock_perror(const char *prefix, int error_code);
const char *vsock_strerror(int error_code);

/* Constants */
#define VSOCK_MAX_PORT 65535
#define VSOCK_MIN_PORT 1
#define VSOCK_DEFAULT_CONNECT_TIMEOUT 30
#define VSOCK_DEFAULT_BUFFER_SIZE (32 * 1024)
#define VSOCK_MAX_ADDRESS_LEN 64

/* CID ranges and special values */
#define VSOCK_CID_RESERVED_START 0
#define VSOCK_CID_RESERVED_END 2
#define VSOCK_CID_DYNAMIC_START 3
#define VSOCK_CID_DYNAMIC_END 0xFFFFFFFF

#else
/* No VSOCK support stubs */
#define is_vsock_supported() 0
#define parse_vsock_cid(cid_str) 0
#define parse_vsock_port(port_str) 0
#define create_vsock_socket() -1
#define bind_vsock_socket(sock, cid, port) -1
#define listen_vsock_socket(sock, backlog) -1
#define connect_vsock_socket(sock, cid, port) -1
#define accept_vsock_connection(listen_sock, client_addr, addr_len) -1
#define set_vsock_buffer_size(sock, size) -1
#define set_vsock_connect_timeout(sock, timeout) -1
#define get_vsock_peer_cid(sock, cid) -1
#define parse_vsock_address(addr_str, cid, port) -1
#define is_valid_cid(cid) 0
#define is_valid_port(port) 0

struct vsock_config {
    int dummy;
};

#define init_vsock_config(config) (-1)
#define cleanup_vsock_config(config) do {} while(0)
#define parse_vsock_config_option(key, value, config) (-1)

#endif /* HAVE_LINUX_VM_SOCKETS_H */

#endif /* VSOCK_SUPPORT_H */
