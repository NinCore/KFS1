/* socket.h - Socket-based IPC system for process communication */

#ifndef SOCKET_H
#define SOCKET_H

#include "types.h"
#include "process.h"

/* Maximum number of sockets */
#define MAX_SOCKETS 256

/* Maximum socket message size */
#define SOCKET_MSG_SIZE 4096

/* Socket states */
typedef enum {
    SOCKET_STATE_CLOSED = 0,
    SOCKET_STATE_LISTENING,
    SOCKET_STATE_CONNECTED,
    SOCKET_STATE_CONNECTING
} socket_state_t;

/* Socket types */
typedef enum {
    SOCKET_TYPE_STREAM = 1,   /* Connection-oriented (like TCP) */
    SOCKET_TYPE_DGRAM = 2     /* Connectionless (like UDP) */
} socket_type_t;

/* Socket domain/family */
typedef enum {
    SOCKET_AF_UNIX = 1,       /* Unix domain sockets (local IPC) */
    SOCKET_AF_INET = 2        /* Internet domain (for future use) */
} socket_family_t;

/* Socket address for Unix domain sockets */
typedef struct {
    socket_family_t family;
    uint32_t pid;             /* Process ID for addressing */
    uint32_t port;            /* Port number (optional) */
} socket_address_t;

/* Socket message buffer entry */
typedef struct socket_msg {
    uint8_t data[SOCKET_MSG_SIZE];
    size_t length;
    socket_address_t src_addr;
    struct socket_msg *next;
} socket_msg_t;

/* Socket descriptor */
typedef struct socket {
    int fd;                   /* File descriptor / socket ID */
    socket_state_t state;
    socket_type_t type;
    socket_family_t family;

    /* Ownership */
    uint32_t owner_pid;       /* Process that owns this socket */

    /* Address binding */
    socket_address_t local_addr;
    socket_address_t remote_addr;

    /* Connection backlog for listening sockets */
    struct socket *accept_queue[16];
    int accept_queue_head;
    int accept_queue_tail;
    int accept_queue_count;

    /* Message queue for received messages */
    socket_msg_t *msg_queue_head;
    socket_msg_t *msg_queue_tail;
    int msg_count;

    /* Connected peer (for STREAM sockets) */
    struct socket *peer;

    /* Reference count */
    int refcount;
} socket_t;

/* Socket system functions */
void socket_init(void);

/* Socket operations */
int socket_create(uint32_t pid, int family, int type, int protocol);
int socket_bind(int sockfd, socket_address_t *addr);
int socket_listen(int sockfd, int backlog);
int socket_accept(int sockfd, socket_address_t *addr);
int socket_connect(int sockfd, socket_address_t *addr);
int socket_send(int sockfd, const void *buf, size_t len, int flags);
int socket_recv(int sockfd, void *buf, size_t len, int flags);
int socket_close(int sockfd);

/* Helper functions */
socket_t *socket_get_by_fd(int fd);
socket_t *socket_find_by_address(socket_address_t *addr);
int socket_can_read(int sockfd);
int socket_can_write(int sockfd);

/* System calls */
int sys_socket(uint32_t family, uint32_t type, uint32_t protocol, uint32_t unused1, uint32_t unused2);
int sys_bind(uint32_t sockfd, uint32_t addr_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3);
int sys_listen(uint32_t sockfd, uint32_t backlog, uint32_t unused1, uint32_t unused2, uint32_t unused3);
int sys_accept(uint32_t sockfd, uint32_t addr_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3);
int sys_connect(uint32_t sockfd, uint32_t addr_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3);
int sys_send(uint32_t sockfd, uint32_t buf_ptr, uint32_t len, uint32_t flags, uint32_t unused);
int sys_recv(uint32_t sockfd, uint32_t buf_ptr, uint32_t len, uint32_t flags, uint32_t unused);
int sys_socket_close(uint32_t sockfd, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);

#endif /* SOCKET_H */
