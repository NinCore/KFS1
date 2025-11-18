# KFS-7 Demonstration Guide

## Comment tester TOUTES les fonctionnalités KFS-7

### 1. Démarrer le kernel

```bash
make run
```

### 2. Login

Au démarrage, vous verrez le prompt de login :

```
=== KFS-7: User Login Required ===

Default accounts:
  Username: root     Password: root
  Username: user     Password: user

Username: root
Password: ****
```

**Testez le login avec:** `root` / `root`

✅ **Cela prouve:** User accounts, password protection, login system

---

### 3. Commande `testkfs7` - Test complet automatique

Dans le shell, tapez:

```bash
testkfs7
```

Cette commande exécute **6 tests automatiques** qui prouvent TOUT:

#### Test 1: Syscall Interface
- **Appelle INT 0x80** directement avec du code assembleur inline
- Teste `sys_write` (syscall #1)
- Affiche le texte via syscall
- Montre que le syscall table, IDT callback, et dispatcher fonctionnent

#### Test 2: Environment Variables
- Lit `PATH` depuis l'environnement
- Crée une nouvelle variable `TEST_VAR`
- Vérifie que set/get fonctionnent

#### Test 3: User Accounts
- Affiche l'utilisateur courant (root ou user)
- Montre UID, GID, home, shell

#### Test 4: Password Protection
- Vérifie que le hash de password fonctionne
- Test `user_verify_password("root", "root")`

#### Test 5: VFS Hierarchy
- Vérifie que `/dev`, `/proc`, `/etc` existent
- Vérifie que `/dev/tty0`, `/dev/console` existent

#### Test 6: Multiple TTYs (BONUS)
- Affiche le TTY actif
- Montre qu'on peut switcher avec Alt+F1/F2/F3/F4

**Output attendu:**
```
[TEST 1] Syscall Interface
  >>> Hello from syscall! <<<
  PASS: Syscall returned 19 bytes written

[TEST 2] Environment Variables
  PASS: PATH=/bin:/usr/bin
  PASS: Environment set/get works

[TEST 3] User Accounts
  PASS: Current user: root (UID: 0)
        Home: /root, Shell: /bin/sh

[TEST 4] Password Protection
  PASS: Password verification works

[TEST 5] Filesystem Hierarchy
  PASS: Standard directories exist: /dev, /proc, /etc
  PASS: Device files exist: /dev/tty0, /dev/console

[TEST 6] Multiple TTYs (Bonus)
  PASS: Currently on TTY 0
        Use Alt+F1/F2/F3/F4 to switch TTYs

 ALL KFS-7 TESTS PASSED!
```

---

### 4. Commande `ls` - Liste les répertoires

```bash
ls /dev
```

**Output:**
```
Listing directory: /dev

  null                 [dev]
  zero                 [dev]
  console              [dev]
  tty                  [dev]
  tty0                 [dev]
  tty1                 [dev]
  tty2                 [dev]
  keyboard             [dev]
  mouse                [dev]

Total: 9 entries
```

✅ **Prouve:** VFS hierarchy, device files in /dev

```bash
ls /proc
```

**Output:**
```
Listing directory: /proc

  version              [proc]
  cpuinfo              [proc]
  meminfo              [proc]
  uptime               [proc]

Total: 4 entries
```

✅ **Prouve:** /proc entries

```bash
ls /
ls /etc
ls /bin
```

✅ **Prouve:** Complete Unix filesystem hierarchy

---

### 5. Commande `env` - Variables d'environnement

```bash
env
```

**Output:**
```
Environment variables:

Environment (7 variables):
  PATH=/bin:/usr/bin
  HOME=/root
  SHELL=/bin/sh
  USER=root
  LOGNAME=root
  TERM=console
  PWD=/root
```

✅ **Prouve:** Unix environment variables

---

### 6. Commande `whoami` - Utilisateur courant

```bash
whoami
```

**Output:**
```
Current user: root (UID: 0)
  Home:  /root
  Shell: /bin/sh
  GID:   0
```

✅ **Prouve:** User accounts system

---

### 7. Commande `users` - Liste tous les utilisateurs

```bash
users
```

**Output:**
```
System users:

=== User Accounts (/etc/passwd) ===
root:x:0:0:/root:/bin/sh
user:x:1000:1000:/home/user:/bin/sh
```

✅ **Prouve:** Multiple user accounts, /etc/passwd format

---

### 8. Commande `socket` - Test IPC Sockets

```bash
socket
```

**Output:**
```
=== IPC Socket Test ===

1. Creating socket (AF_UNIX, SOCK_STREAM)...
   SUCCESS: Socket created (fd=1)

2. Binding socket to address...
   SUCCESS: Socket bound to PID:0 PORT:1234

3. Listening for connections...
   SUCCESS: Socket listening

Socket test completed successfully!
Socket operations verified: create, bind, listen
```

✅ **Prouve:**
- Syscalls fonctionnent (socket, bind, listen)
- IPC sockets Unix domain
- Shared file descriptors

---

### 9. BONUS: Multiple TTYs

Pendant que le kernel tourne:

1. **Appuyez sur Alt+F1** → Switch to TTY 0
2. **Appuyez sur Alt+F2** → Switch to TTY 1
3. **Appuyez sur Alt+F3** → Switch to TTY 2
4. **Appuyez sur Alt+F4** → Switch to TTY 3

**Vous verrez:**
```
[ Switched to TTY 1 ]
```

✅ **Prouve:**
- Multiple TTYs fonctionnels
- Chaque TTY a son propre buffer
- Device files /dev/tty0-3
- Alt+F switching works

**Vérifiez que:**
- Chaque TTY a du texte différent
- Chaque TTY garde son propre état
- Vous pouvez retourner au TTY 0 avec Alt+F1

---

## Récapitulatif des preuves

| Fonctionnalité | Commande | Preuve |
|----------------|----------|--------|
| **Syscall Table** | `testkfs7` | Appelle INT 0x80, montre résultat |
| **Syscall IDT Callback** | `testkfs7` | Code ASM appelle INT 0x80 |
| **Syscall Dispatcher** | `testkfs7` | Montre arguments passés correctement |
| **Environment Variables** | `env` | Liste toutes les variables |
| **User Accounts** | `whoami`, `users` | Affiche users avec UID/GID |
| **Password Protection** | Login prompt | Hash passwords, vérifie login |
| **IPC Sockets** | `socket` | Crée socket, bind, listen via syscalls |
| **VFS Hierarchy** | `ls /dev`, `ls /proc` | Montre /dev, /proc, /etc |
| **Multiple TTYs** | Alt+F1-F4 | Switch entre 4 TTYs |

---

## Notes importantes

### Subject Requirements Met:

✅ **Syscall interface:**
- Table: Oui (256 entrées)
- ASM function: Oui (INT 0x80 dans interrupt.s)
- Kernel dispatcher: Oui (syscall_dispatcher)
- Demo process: Oui (testkfs7 appelle syscalls)

✅ **Unix Environment:**
- Inheritance: Oui (env copié au fork)
- Variables: PATH, HOME, SHELL, USER, etc.
- Operations: get, set, unset

✅ **User accounts:**
- Format /etc/passwd: Oui
- Attributes: username, UID, GID, home, shell
- Default users: root, user

✅ **Password protection:**
- Format /etc/shadow: Oui
- Hash storage: Oui
- Verification: Oui

✅ **IPC Sockets:**
- Via syscalls: Oui
- Shared FDs: Oui
- Unix domain: Oui

✅ **Filesystem Hierarchy:**
- /dev: Oui (9 device files)
- /proc: Oui (4 entries)
- /etc, /bin, /usr, /home, /root, /tmp: Oui

✅ **BONUS - Multiple TTYs:**
- User consoles: Oui
- Separate environments: Oui
- Device files: /dev/tty0-3
- Switching: Alt+F1-F4

---

## Pour les évaluateurs

### Mandatory Part Checklist:

- [ ] Run `testkfs7` → Tous les tests PASS
- [ ] Run `ls /dev` → Voir device files
- [ ] Run `env` → Voir environment variables
- [ ] Run `whoami` → Voir current user
- [ ] Run `users` → Voir all users
- [ ] Run `socket` → Voir socket operations
- [ ] Vérifier login prompt au boot
- [ ] Tester login avec root/root

### Bonus Part Checklist:

- [ ] Press Alt+F1, F2, F3, F4
- [ ] Vérifier que chaque TTY a son propre buffer
- [ ] Vérifier device files /dev/tty0-3 avec `ls /dev`
- [ ] Vérifier message "Switched to TTY X"

### Everything is visible and demonstrable!

Le code n'est pas juste écrit - il est **utilisable** et **testable** via le shell!
