# KFS-7: Syscalls, Sockets and env - Implementation Guide

## Overview

This document describes the KFS-7 implementation which adds system calls, user accounts, environment variables, IPC sockets, and a Unix-like filesystem hierarchy to the kernel.

## Mandatory Features Implemented

### 1. Syscall Interface

**Files:** `include/syscall.h`, `src/syscall.c`, `src/interrupt.s`

- **Syscall Table**: Array-based table storing function pointers for up to 256 syscalls
- **IDT Integration**: INT 0x80 configured in IDT to handle syscalls
- **ASM Handler**: `isr128` in interrupt.s catches INT 0x80 and calls the C dispatcher
- **Dispatcher**: `syscall_dispatcher()` extracts syscall number from EAX and arguments from EBX, ECX, EDX, ESI, EDI
- **Return Value**: Result placed back in EAX register

**Registered Syscalls:**
- SYS_EXIT (0): Exit process
- SYS_WRITE (1): Write to file descriptor
- SYS_READ (2): Read from file descriptor
- SYS_GETUID (11): Get current user ID
- SYS_SOCKET (14-20): Socket operations (see below)
- SYS_GETENV (21): Get environment variable
- SYS_SETENV (22): Set environment variable
- SYS_UNSETENV (23): Unset environment variable
- SYS_SETUID (24): Set user ID (root only)

### 2. Unix Environment Variables

**Files:** `include/env.h`, `src/env.c`

- **Environment Table**: Each process has its own environment (64 variables max)
- **Name/Value Pairs**: Variables stored as name=value pairs
- **Inheritance**: Child processes inherit parent's environment on fork
- **Default Environment**: Includes PATH, HOME, SHELL, USER, LOGNAME, TERM, PWD
- **Operations**: env_get(), env_set(), env_unset()
- **Syscalls**: sys_getenv(), sys_setenv(), sys_unsetenv()

### 3. User Accounts System

**Files:** `include/user.h`, `src/user.c`

Implements Unix-style user accounts similar to /etc/passwd:

- **User Attributes**:
  - Username
  - UID (User ID)
  - GID (Group ID)
  - Home directory
  - Login shell

- **Default Users**:
  - root (uid=0, password="root")
  - user (uid=1000, password="user")

- **Operations**: user_create(), user_delete(), user_get_by_name(), user_get_by_uid()

### 4. Password Protection

**Files:** `include/user.h`, `src/user.c`

Implements password hashing similar to /etc/shadow:

- **Hashing**: Simple hash function for demonstration (NOT cryptographically secure)
- **Storage**: Passwords stored as hex hashes separate from user accounts
- **Verification**: user_verify_password() compares hashes
- **Login System**: Interactive login with username/password prompts
- **Security Note**: Uses simple hash for demonstration - production systems should use bcrypt/scrypt/argon2

### 5. Inter-Process Communication (IPC) Sockets

**Files:** `include/socket.h`, `src/socket.c`

Full Unix domain socket implementation:

- **Socket Types**: SOCK_STREAM (connection-oriented), SOCK_DGRAM (connectionless)
- **Socket Family**: AF_UNIX (local IPC)
- **Operations**:
  - socket_create(): Create socket
  - socket_bind(): Bind to address
  - socket_listen(): Listen for connections
  - socket_accept(): Accept incoming connection
  - socket_connect(): Connect to remote socket
  - socket_send()/socket_recv(): Data transfer
- **Message Queue**: Buffered message delivery
- **Syscalls**: All operations exposed via syscalls (SYS_SOCKET, SYS_BIND, etc.)

### 6. Unix Filesystem Hierarchy

**Files:** `include/vfs_hierarchy.h`, `src/vfs_hierarchy.c`

Virtual filesystem implementing Unix directory structure:

- **Root Directories**:
  - /dev: Device files
  - /proc: Process information
  - /etc: Configuration files
  - /bin, /usr/bin: Binaries
  - /home: User home directories
  - /root: Root's home
  - /tmp: Temporary files

- **Device Files** (in /dev):
  - null, zero: Null devices
  - console, tty, tty0-2: TTYs
  - keyboard, mouse: Input devices

- **Proc Entries** (in /proc):
  - version, cpuinfo, meminfo, uptime

- **Operations**: vfs_create_directory(), vfs_find_file(), vfs_list_directory()

## Bonus Features Implemented

### Multiple TTYs (Virtual Terminals)

**Files:** `include/tty.h`, `src/tty.c`

- **TTY Count**: 4 virtual terminals (TTY 0-3)
- **Features**:
  - Separate VGA buffer for each TTY
  - Independent cursor positions
  - Per-TTY input buffers
  - User session tracking
- **Switching**: Alt+F1/F2/F3/F4 switches between TTYs
- **Integration**: Keyboard handler detects function keys and calls tty_switch()

### Login System

**Files:** `include/login.h`, `src/login.c`

- **Interactive Login**: Username and password prompts at boot
- **Password Masking**: Shows asterisks instead of password
- **Multiple Attempts**: Allows 3 login attempts
- **TTY Integration**: Sets logged-in user for current TTY
- **Default Accounts**: Displays available accounts at login

## Architecture

### System Initialization Order (kernel.c)

1. VGA, GDT, IDT initialization
2. Signal and syscall systems
3. Memory management (paging, kmalloc, vmalloc)
4. Process system
5. Socket system
6. **Environment system** (KFS-7)
7. **User accounts** (KFS-7)
8. **VFS hierarchy** (KFS-7)
9. **TTY system** (KFS-7 BONUS)
10. **Login system** (KFS-7)
11. Keyboard and interrupts
12. Shell

### Process Structure Enhancement

Added to `process_t` structure:
- `struct env_table *environment`: Pointer to process environment

### Syscall Flow

```
User Space                 Kernel Space
----------                 ------------
process calls
syscall wrapper  ->  INT 0x80  ->  isr128 (ASM)
                                      |
                                      v
                               syscall_dispatcher()
                                      |
                                      v
                               syscall_handlers[num]()
                                      |
                                      v
                               return value in EAX
```

## Testing

### Manual Testing

1. **Login System**:
   - Boot kernel
   - Enter "root" / "root" or "user" / "user"
   - Verify successful login

2. **TTY Switching**:
   - Press Alt+F1 to Alt+F4
   - Verify screen switches
   - Verify each TTY maintains separate state

3. **Environment** (via shell if implemented):
   - Call sys_getenv() to read variables
   - Call sys_setenv() to modify
   - Fork process and verify inheritance

4. **Sockets** (via process creation):
   - Create two processes
   - One calls socket(), bind(), listen(), accept()
   - Other calls socket(), connect()
   - Exchange messages with send()/recv()

## Building

```bash
make clean
make kernel
make iso
make run
```

## Files Added for KFS-7

### Headers
- `include/env.h`: Environment variables
- `include/user.h`: User accounts and passwords
- `include/vfs_hierarchy.h`: Virtual filesystem
- `include/tty.h`: Multiple TTYs
- `include/login.h`: Login system

### Source Files
- `src/env.c`: Environment implementation
- `src/user.c`: User management
- `src/vfs_hierarchy.c`: VFS implementation
- `src/tty.c`: TTY implementation
- `src/login.c`: Login system

### Modified Files
- `include/syscall.h`: Added new syscall numbers
- `src/syscall.c`: Registered new syscalls
- `include/process.h`: Added environment pointer
- `src/kernel.c`: Added KFS-7 initialization
- `src/shell.c`: Added TTY switching support
- `include/vga.h`: Added helper functions for TTY

## Security Considerations

1. **Password Hashing**: Current implementation uses a simple hash for demonstration. Production systems must use:
   - bcrypt
   - scrypt
   - argon2
   - PBKDF2 with high iteration count

2. **UID 0 Privileges**: Only root (UID 0) can call sys_setuid()

3. **Process Isolation**: Each process has its own environment that doesn't affect others

4. **Socket Security**: Sockets verified by PID, preventing unauthorized access

## Compliance with Subject Requirements

### Mandatory
- ✅ Syscall table implemented
- ✅ ASM function for IDT callback (INT 0x80)
- ✅ Kernel-side syscall dispatcher
- ✅ Demo process using syscalls
- ✅ Unix environment variables with inheritance
- ✅ User accounts (/etc/passwd style)
- ✅ Password protection (/etc/shadow style)
- ✅ IPC sockets with shared file descriptors
- ✅ Unix filesystem hierarchy (/dev, /proc, /etc, etc.)

### Bonus
- ✅ Multiple TTYs with separate consoles
- ✅ User-specific environments per TTY
- ✅ TTY files in /dev (tty0, tty1, tty2, tty3)
- ✅ Alt+F1/F2/F3/F4 TTY switching

## Future Enhancements

1. Proper cryptographic password hashing
2. File permissions and access control
3. More complete /proc entries (per-process info)
4. Full ELF binary loading with environment passing
5. execve() syscall with environment
6. Network sockets (AF_INET)
7. More sophisticated TTY features (scrollback, colors per TTY)
8. PAM-like authentication modules
