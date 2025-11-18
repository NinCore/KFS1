# KFS-5 Process Implementation

## Problème Résolu

### Page Fault à 0x10002000

L'erreur de Page Fault se produisait car:

1. **Système de paging limité**: Le code original ne mappait que les premiers 8MB de mémoire
2. **Pas d'allocation dynamique de page tables**: La fonction `paging_map_page()` ne pouvait pas créer de nouvelles page tables
3. **Adresse 0x10002000 non mappée**: Cette adresse (~256MB) était en dehors de la zone identity-mapped
4. **Infrastructure processus manquante**: Aucune structure ni fonction pour gérer les processus

## Solutions Implémentées

### 1. Structures de Données (include/process.h, src/process.c)

- **Process Control Block (PCB)**: Structure complète avec PID, état, parent/enfants, mémoire, signaux, UID
- **Contexte processus**: Sauvegarde des registres pour le multitasking
- **Queue de signaux**: File d'attente pour les signaux par processus

### 2. Amélioration du Système de Paging (src/paging.c)

**Nouvelles fonctions critiques**:

- `paging_create_directory()`: Crée un page directory pour un processus
- `paging_destroy_directory()`: Libère un page directory et ses page tables
- `paging_clone_directory()`: Clone un directory (pour fork)
- `paging_map_page_in_directory()`: **CLEF** - Alloue dynamiquement des page tables si nécessaire
- `paging_switch_directory()`: Change le contexte mémoire

**Changement principal**: Au lieu de faire `kernel_panic` quand une page table n'existe pas, on l'alloue dynamiquement avec `kmalloc()`.

### 3. Gestion des Processus (src/process.c)

**Fonctions principales**:

- `process_create()`: Crée un nouveau processus avec son propre espace d'adressage
- `process_fork()`: Clone un processus (copy-on-write simplifiée)
- `process_exit()`: Termine un processus et libère ses ressources
- `process_wait()`: Attend la fin d'un processus enfant
- `process_kill()`: Envoie un signal à un processus

**Gestion des signaux par processus**:

- `process_signal_register()`: Enregistre un handler de signal pour un processus
- `process_signal_send()`: Envoie un signal à un processus (le met en queue)
- `process_signal_process()`: Traite tous les signaux en attente

### 4. Syscalls (src/syscall.c)

Nouveaux syscalls enregistrés:

- `SYS_FORK` (9): Fork le processus courant
- `SYS_WAIT` (10): Attend un processus enfant
- `SYS_GETUID` (11): Obtient l'UID de l'utilisateur
- `SYS_KILL` (8): Envoie un signal à un processus

### 5. Commandes de Test (src/shell.c)

Trois nouvelles commandes pour tester:

- `process`: Teste la création de processus
- `fork`: Teste le syscall fork
- `psignal`: Teste l'envoi de signaux aux processus

## Architecture

```
Kernel
├── Process System (KFS-5)
│   ├── Process table (256 processus max)
│   ├── Allocation PID
│   └── Gestion parent-enfant
│
├── Memory Management (amélioré)
│   ├── Page directories par processus
│   ├── Allocation dynamique de page tables
│   └── Isolation mémoire entre processus
│
└── Signal System (par processus)
    ├── Handlers spécifiques par processus
    └── Queue de signaux par processus
```

## Comment Tester

1. **Compiler**: `make clean && make kernel`

2. **Tester dans le shell**:
   ```
   # Créer un processus
   process

   # Tester fork
   fork

   # Tester les signaux de processus
   psignal
   ```

## Caractéristiques Clés

### Séparation Mémoire
- Chaque processus a son propre page directory
- Les 8 premiers MB (kernel) sont partagés (identity-mapped)
- Stack utilisateur à 0x10000000 (maintenant correctement mappé!)

### Signaux par Processus
- Chaque processus a sa propre file d'attente de signaux
- Handlers configurables par processus
- Traitement asynchrone des signaux

### Relations Parent-Enfant
- Arbre de processus avec pointeurs parent/enfants
- Fork crée une relation parent-enfant
- Wait permet au parent d'attendre ses enfants

## Ce qui Reste à Faire (Bonus)

- [ ] Multitasking réel avec scheduler
- [ ] mmap pour accès à la mémoire virtuelle
- [ ] Sections BSS et data dans les processus
- [ ] Copy-on-write pour fork (actuellement copie complète)
- [ ] Support des threads

## Fichiers Modifiés/Créés

### Nouveaux fichiers:
- `include/process.h`
- `src/process.c`
- `KFS5_IMPLEMENTATION.md` (ce fichier)

### Fichiers modifiés:
- `include/paging.h` - Ajout fonctions pour processus
- `src/paging.c` - Allocation dynamique de page tables
- `include/syscall.h` - Nouveaux syscalls
- `src/syscall.c` - Enregistrement des syscalls processus
- `src/kernel.c` - Initialisation du système de processus
- `src/shell.c` - Commandes de test

## Références

- KFS-5 Subject PDF
- OSDev Wiki - Process Management
- OSDev Wiki - Paging
