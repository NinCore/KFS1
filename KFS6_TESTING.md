# KFS-6 - Guide de Test du Système de Fichiers

## 🎯 Fonctionnalités Implémentées

### ✅ MANDATORY
- **Driver IDE** - Lecture/écriture de disques ATA/IDE
- **Parser EXT2** - Lecture complète du filesystem EXT2
- **VFS** - Virtual File System avec montage/démontage
- **Commandes filesystem**: `cat`, `pwd`, `cd`, `ls`
- **PWD par processus** - Chaque processus a son répertoire courant

### ⭐ BONUS
- **Mount/Unmount** - Montage de partitions EXT2 multiples
- **Partitions multiples** - Support de 4 périphériques IDE
- **Système utilisateurs** - Login/password avec authentification

---

## 🚀 Compilation et Lancement

### Option 1 : Sans disque (filesystem virtuel en mémoire)
```bash
make clean
make run
```

Le VFS créera automatiquement:
- `/` - Répertoire racine
- `/dev`, `/tmp`, `/home` - Répertoires système
- `/readme.txt` - Fichier de test

### Option 2 : Avec disque EXT2 (recommandé pour tests complets)
```bash
make clean
make run-disk
```

Cette commande va:
1. Compiler le kernel
2. Créer une image disque EXT2 de 10 MB
3. La remplir avec des fichiers de test
4. Lancer QEMU avec le disque attaché

---

## 📁 Commandes Filesystem Disponibles

### Commandes de base
```bash
ls              # Liste le contenu du répertoire courant
ls /            # Liste le contenu de la racine
ls /home        # Liste le contenu de /home

pwd             # Affiche le répertoire courant

cd /home        # Change vers /home
cd /            # Retour à la racine
cd              # Retour à la racine (sans argument)

cat /readme.txt       # Affiche le contenu d'un fichier
cat /home/hello.txt   # Lit un fichier dans un sous-répertoire
```

### Commandes BONUS - Mount/Unmount
```bash
mount                 # Affiche les filesystems montés
mount hda0 /mnt      # Monte le disque primary master sur /mnt
umount /mnt          # Démonte /mnt
```

Périphériques disponibles:
- `hda0` - Primary Master (IDE 0:0)
- `hda1` - Primary Slave (IDE 0:1)
- `hdb0` - Secondary Master (IDE 1:0)
- `hdb1` - Secondary Slave (IDE 1:1)

### Commandes BONUS - Utilisateurs
```bash
whoami              # Affiche l'utilisateur courant (root par défaut)
login user          # Se connecter en tant que 'user'
login admin         # Se connecter en tant que 'admin'
login guest         # Se connecter en tant que 'guest'
```

**Utilisateurs disponibles:**
| Username | Password | UID  | Description |
|----------|----------|------|-------------|
| root     | root     | 0    | Super utilisateur |
| admin    | admin    | 1    | Administrateur |
| user     | user     | 1000 | Utilisateur normal |
| guest    | guest    | 1001 | Invité |

---

## 🧪 Scénario de Test Complet

### Test 1 : Filesystem Virtuel
```bash
make run

# Dans le shell KFS:
ls /                    # Doit afficher: dev/, tmp/, home/, readme.txt
cat /readme.txt         # Affiche le contenu du fichier de bienvenue
cd /home
pwd                     # Affiche: /home
cd /
pwd                     # Affiche: /
```

### Test 2 : Avec Disque EXT2
```bash
make run-disk

# Dans le shell KFS:
ls /                    # Liste le filesystem virtuel
mount hda0 /mnt         # Monte le disque EXT2
ls /mnt                 # Liste le contenu du disque
cat /mnt/readme.txt     # Lit un fichier du disque
cat /mnt/welcome.txt    # Lit un autre fichier
cd /mnt/home
pwd                     # Affiche: /mnt/home
ls                      # Liste les fichiers dans /mnt/home
cat hello.txt           # Lit un fichier (chemin relatif)
cd /
umount /mnt             # Démonte proprement
```

### Test 3 : Système Utilisateurs
```bash
whoami                  # Affiche: root (UID: 0)
login user              # Login en tant que user
whoami                  # Affiche: user (UID: 1000)
login admin             # Login en tant que admin
whoami                  # Affiche: admin (UID: 1)
```

---

## 📝 Structure du Disque EXT2 Créé

```
disk.img (10 MB, EXT2)
├── welcome.txt          "Welcome to KFS-6 Filesystem!"
├── test.txt             "This is a test file on EXT2 disk"
├── readme.txt           Fichier de documentation complète
├── /home/
│   └── hello.txt        "Hello from /home directory!"
├── /dev/               (vide)
├── /tmp/               (vide)
└── /etc/               (vide)
```

---

## 🔧 Créer un Disque EXT2 Personnalisé

```bash
# Manuellement:
chmod +x create_disk.sh
./create_disk.sh

# Ou via Make:
make disk
```

Pour ajouter vos propres fichiers:
```bash
# Monter l'image
sudo mount -o loop disk.img /mnt

# Ajouter des fichiers
sudo cp myfile.txt /mnt/
sudo mkdir /mnt/mydir
sudo cp -r mydir/* /mnt/mydir/

# Démonter
sudo umount /mnt
```

---

## 🐛 Dépannage

### Problème: Commandes "not found"
**Solution**: Recompiler complètement
```bash
make clean
make kernel
```

### Problème: "No EXT2 filesystem found"
**Vérifications**:
1. Le disque est-il bien créé? `ls -lh disk.img`
2. Est-il bien formaté en EXT2? `file disk.img`
3. QEMU est-il lancé avec le bon paramètre? Vérifier `-drive file=disk.img`

### Problème: Permission denied lors de la création du disque
**Solution**: Le script nécessite sudo pour monter l'image
```bash
sudo ./create_disk.sh
```

---

## 📊 Commandes de Debug Disponibles

```bash
help            # Liste toutes les commandes disponibles
mem             # Affiche les informations mémoire
kstats          # Statistiques du heap kernel
vstats          # Statistiques mémoire virtuelle
process         # Teste le système de processus
```

---

## ✅ Checklist de Validation KFS-6

### MANDATORY
- [x] Driver IDE fonctionnel
- [x] Parser EXT2 complet
- [x] VFS avec structure d'arbre
- [x] Commande `cat` - affiche contenu fichier
- [x] Commande `pwd` - affiche répertoire courant
- [x] Commande `cd` - change de répertoire
- [x] Commande `ls` - liste contenu répertoire
- [x] PWD par processus

### BONUS
- [x] Mount/Unmount de filesystems
- [x] Support multiples partitions (4 IDE devices)
- [x] Système utilisateurs avec login/password
- [x] Filesystem virtuel fonctionnel sans disque

---

## 🎉 Résultat Final

**KFS-6 MANDATORY**: ✅ 100%
**KFS-6 BONUS 1** (Mount/Unmount): ✅ 100%
**KFS-6 BONUS 2** (Partitions multiples): ✅ 100%
**KFS-6 BONUS 3** (Login/Password): ✅ 100%

Tous les objectifs sont atteints avec un filesystem virtuel qui fonctionne immédiatement au boot, plus la possibilité de monter des vrais disques EXT2!
