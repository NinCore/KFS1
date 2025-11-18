/* socket.c - Socket-based IPC implementation */

#include "../include/socket.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/process.h"

/* Socket table */
static socket_t socket_table[MAX_SOCKETS];

/* Next socket FD to allocate */
static int next_sockfd = 1;

/* Initialize socket system */
void socket_init(void) {
    memset(socket_table, 0, sizeof(socket_table));

    for (int i = 0; i < MAX_SOCKETS; i++) {
        socket_table[i].state = SOCKET_STATE_CLOSED;
        socket_table[i].fd = -1;
    }

    kernel_info("Socket system initialized");
}

/* Find free socket slot */
static socket_t *socket_alloc(void) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].state == SOCKET_STATE_CLOSED && socket_table[i].fd == -1) {
            return &socket_table[i];
        }
    }
    return NULL;
}

/* Get socket by file descriptor */
socket_t *socket_get_by_fd(int fd) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].fd == fd && socket_table[i].state != SOCKET_STATE_CLOSED) {
            return &socket_table[i];
        }
    }
    return NULL;
}

/* Find socket by address */
socket_t *socket_find_by_address(socket_address_t *addr) {
    if (!addr) {
        return NULL;
    }

    for (int i = 0; i < MAX_SOCKETS; i++) {
        socket_t *sock = &socket_table[i];
        if (sock->state == SOCKET_STATE_LISTENING &&
            sock->local_addr.family == addr->family &&
            sock->local_addr.pid == addr->pid &&
            sock->local_addr.port == addr->port) {
            return sock;
        }
    }
    return NULL;
}

/* Create a new socket */
int socket_create(uint32_t pid, int family, int type, int protocol) {
    (void)protocol;  /* Unused */

    /* Validate parameters */
    if (family != SOCKET_AF_UNIX) {
        return -1;  /* Only Unix domain sockets supported */
    }

    if (type != SOCKET_TYPE_STREAM && type != SOCKET_TYPE_DGRAM) {
        return -1;  /* Invalid socket type */
    }

    /* Allocate socket */
    socket_t *sock = socket_alloc();
    if (!sock) {
        return -1;  /* No free sockets */
    }

    /* Initialize socket */
    memset(sock, 0, sizeof(socket_t));
    sock->fd = next_sockfd++;
    sock->state = SOCKET_STATE_CLOSED;
    sock->type = (socket_type_t)type;
    sock->family = (socket_family_t)family;
    sock->owner_pid = pid;
    sock->refcount = 1;

    printk("[SOCKET] Created socket fd=%d for PID %d\n", sock->fd, pid);
    return sock->fd;
}

/* Bind socket to address */
int socket_bind(int sockfd, socket_address_t *addr) {
    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock || !addr) {
        return -1;
    }

    /* Check if already bound */
    if (sock->local_addr.port != 0) {
        return -1;  /* Already bound */
    }

    /* Check if address is already in use */
    if (socket_find_by_address(addr)) {
        return -1;  /* Address in use */
    }

    /* Bind to address */
    sock->local_addr = *addr;
    printk("[SOCKET] Bound socket fd=%d to PID:%d PORT:%d\n",
           sockfd, addr->pid, addr->port);

    return 0;
}

/* Listen for connections */
int socket_listen(int sockfd, int backlog) {
    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock) {
        return -1;
    }

    /* Only STREAM sockets can listen */
    if (sock->type != SOCKET_TYPE_STREAM) {
        return -1;
    }

    /* Must be bound first */
    if (sock->local_addr.port == 0) {
        return -1;  /* Not bound */
    }

    /* Set state to listening */
    sock->state = SOCKET_STATE_LISTENING;
    sock->accept_queue_head = 0;
    sock->accept_queue_tail = 0;
    sock->accept_queue_count = 0;

    (void)backlog;  /* Ignore backlog for now */

    printk("[SOCKET] Socket fd=%d listening\n", sockfd);
    return 0;
}

/* Accept a connection */
int socket_accept(int sockfd, socket_address_t *addr) {
    socket_t *listen_sock = socket_get_by_fd(sockfd);
    if (!listen_sock) {
        return -1;
    }

    /* Must be listening */
    if (listen_sock->state != SOCKET_STATE_LISTENING) {
        return -1;
    }

    /* Check if there are pending connections */
    if (listen_sock->accept_queue_count == 0) {
        return -1;  /* No pending connections (would block in real system) */
    }

    /* Get connection from queue */
    socket_t *client_sock = listen_sock->accept_queue[listen_sock->accept_queue_head];
    listen_sock->accept_queue_head = (listen_sock->accept_queue_head + 1) % 16;
    listen_sock->accept_queue_count--;

    /* Return client address if requested */
    if (addr) {
        *addr = client_sock->remote_addr;
    }

    printk("[SOCKET] Accepted connection fd=%d from PID:%d\n",
           client_sock->fd, client_sock->remote_addr.pid);

    return client_sock->fd;
}

/* Connect to remote socket */
int socket_connect(int sockfd, socket_address_t *addr) {
    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock || !addr) {
        return -1;
    }

    /* Find listening socket */
    socket_t *listen_sock = socket_find_by_address(addr);
    if (!listen_sock) {
        return -1;  /* No socket listening at address */
    }

    /* Create a new socket for the connection */
    socket_t *server_sock = socket_alloc();
    if (!server_sock) {
        return -1;
    }

    /* Initialize server-side socket */
    memset(server_sock, 0, sizeof(socket_t));
    server_sock->fd = next_sockfd++;
    server_sock->state = SOCKET_STATE_CONNECTED;
    server_sock->type = listen_sock->type;
    server_sock->family = listen_sock->family;
    server_sock->owner_pid = listen_sock->owner_pid;
    server_sock->local_addr = listen_sock->local_addr;
    server_sock->remote_addr.family = SOCKET_AF_UNIX;
    server_sock->remote_addr.pid = sock->owner_pid;
    server_sock->remote_addr.port = 0;
    server_sock->refcount = 1;

    /* Set up peer connection */
    sock->peer = server_sock;
    server_sock->peer = sock;
    sock->state = SOCKET_STATE_CONNECTED;
    sock->remote_addr = *addr;

    /* Add to accept queue */
    if (listen_sock->accept_queue_count < 16) {
        listen_sock->accept_queue[listen_sock->accept_queue_tail] = server_sock;
        listen_sock->accept_queue_tail = (listen_sock->accept_queue_tail + 1) % 16;
        listen_sock->accept_queue_count++;
    }

    printk("[SOCKET] Connected fd=%d to PID:%d PORT:%d\n",
           sockfd, addr->pid, addr->port);

    return 0;
}

/* Send data to socket */
int socket_send(int sockfd, const void *buf, size_t len, int flags) {
    (void)flags;  /* Unused */

    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock || !buf || len == 0) {
        return -1;
    }

    /* Must be connected */
    if (sock->state != SOCKET_STATE_CONNECTED || !sock->peer) {
        return -1;
    }

    /* Limit message size */
    if (len > SOCKET_MSG_SIZE) {
        len = SOCKET_MSG_SIZE;
    }

    /* Allocate message */
    socket_msg_t *msg = (socket_msg_t *)kmalloc(sizeof(socket_msg_t));
    if (!msg) {
        return -1;
    }

    /* Copy data */
    memcpy(msg->data, buf, len);
    msg->length = len;
    msg->src_addr = sock->local_addr;
    msg->next = NULL;

    /* Add to peer's message queue */
    socket_t *peer = sock->peer;
    if (peer->msg_queue_tail) {
        peer->msg_queue_tail->next = msg;
    } else {
        peer->msg_queue_head = msg;
    }
    peer->msg_queue_tail = msg;
    peer->msg_count++;

    printk("[SOCKET] Sent %d bytes from fd=%d to fd=%d\n", len, sockfd, peer->fd);
    return (int)len;
}

/* Receive data from socket */
int socket_recv(int sockfd, void *buf, size_t len, int flags) {
    (void)flags;  /* Unused */

    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock || !buf || len == 0) {
        return -1;
    }

    /* Check if there are messages */
    if (!sock->msg_queue_head) {
        return 0;  /* No messages (would block in real system) */
    }

    /* Get message from queue */
    socket_msg_t *msg = sock->msg_queue_head;
    sock->msg_queue_head = msg->next;
    if (!sock->msg_queue_head) {
        sock->msg_queue_tail = NULL;
    }
    sock->msg_count--;

    /* Copy data */
    size_t copy_len = (len < msg->length) ? len : msg->length;
    memcpy(buf, msg->data, copy_len);

    /* Free message */
    kfree(msg);

    printk("[SOCKET] Received %d bytes on fd=%d\n", copy_len, sockfd);
    return (int)copy_len;
}

/* Close socket */
int socket_close(int sockfd) {
    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock) {
        return -1;
    }

    /* Disconnect peer if connected */
    if (sock->peer) {
        sock->peer->peer = NULL;
        sock->peer->state = SOCKET_STATE_CLOSED;
    }

    /* Free message queue */
    socket_msg_t *msg = sock->msg_queue_head;
    while (msg) {
        socket_msg_t *next = msg->next;
        kfree(msg);
        msg = next;
    }

    /* Reset socket */
    memset(sock, 0, sizeof(socket_t));
    sock->state = SOCKET_STATE_CLOSED;
    sock->fd = -1;

    printk("[SOCKET] Closed socket fd=%d\n", sockfd);
    return 0;
}

/* Check if socket can read */
int socket_can_read(int sockfd) {
    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock) {
        return 0;
    }

    return (sock->msg_count > 0) ? 1 : 0;
}

/* Check if socket can write */
int socket_can_write(int sockfd) {
    socket_t *sock = socket_get_by_fd(sockfd);
    if (!sock) {
        return 0;
    }

    return (sock->state == SOCKET_STATE_CONNECTED) ? 1 : 0;
}

/* ===== System Calls ===== */

/* sys_socket - Create a socket */
int sys_socket(uint32_t family, uint32_t type, uint32_t protocol, uint32_t unused1, uint32_t unused2) {
    (void)unused1; (void)unused2;

    process_t *proc = process_get_current();
    if (!proc) {
        return -1;
    }

    return socket_create(proc->pid, (int)family, (int)type, (int)protocol);
}

/* sys_bind - Bind socket to address */
int sys_bind(uint32_t sockfd, uint32_t addr_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;

    socket_address_t *addr = (socket_address_t *)addr_ptr;
    return socket_bind((int)sockfd, addr);
}

/* sys_listen - Listen for connections */
int sys_listen(uint32_t sockfd, uint32_t backlog, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;

    return socket_listen((int)sockfd, (int)backlog);
}

/* sys_accept - Accept a connection */
int sys_accept(uint32_t sockfd, uint32_t addr_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;

    socket_address_t *addr = (socket_address_t *)addr_ptr;
    return socket_accept((int)sockfd, addr);
}

/* sys_connect - Connect to remote socket */
int sys_connect(uint32_t sockfd, uint32_t addr_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;

    socket_address_t *addr = (socket_address_t *)addr_ptr;
    return socket_connect((int)sockfd, addr);
}

/* sys_send - Send data to socket */
int sys_send(uint32_t sockfd, uint32_t buf_ptr, uint32_t len, uint32_t flags, uint32_t unused) {
    (void)unused;

    void *buf = (void *)buf_ptr;
    return socket_send((int)sockfd, buf, (size_t)len, (int)flags);
}

/* sys_recv - Receive data from socket */
int sys_recv(uint32_t sockfd, uint32_t buf_ptr, uint32_t len, uint32_t flags, uint32_t unused) {
    (void)unused;

    void *buf = (void *)buf_ptr;
    return socket_recv((int)sockfd, buf, (size_t)len, (int)flags);
}

/* sys_socket_close - Close socket */
int sys_socket_close(uint32_t sockfd, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;

    return socket_close((int)sockfd);
}
