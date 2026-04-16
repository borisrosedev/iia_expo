# Documentation — Simulation du Handshake TCP en C++

## Table des matières

1. [Contexte : C'est quoi le handshake TCP ?](#1-contexte--cest-quoi-le-handshake-tcp-)
2. [Architecture du projet](#2-architecture-du-projet)
3. [Les fichiers expliqués](#3-les-fichiers-expliqués)
   - [common.hpp](#31-commonhpp)
   - [server.cpp](#32-servercpp)
   - [client.cpp](#33-clientcpp)
   - [Makefile](#34-makefile)
4. [Le déroulement complet du handshake](#4-le-déroulement-complet-du-handshake)
5. [Commandes pour lancer le projet](#5-commandes-pour-lancer-le-projet)
6. [Exemple de sortie](#6-exemple-de-sortie)
7. [Points techniques importants](#7-points-techniques-importants)

---

## 1. Contexte : C'est quoi le handshake TCP ?

TCP (Transmission Control Protocol) est un protocole de communication **orienté connexion**. Avant d'échanger des données, les deux parties (client et serveur) doivent **établir une connexion** via un processus appelé le **3-way handshake** (poignée de main en 3 étapes).

Dans la vraie vie, ce handshake est géré automatiquement par le système d'exploitation (la pile réseau du noyau Linux/Windows). Ici, on le **simule manuellement** en C++ pour comprendre exactement ce qui se passe.

### Pourquoi simuler et pas utiliser les vraies sockets TCP ?

Avec les sockets TCP classiques (`SOCK_STREAM`), le système d'exploitation gère le handshake en coulisse. On ne voit rien. Ici, on utilise des **sockets UDP** (`SOCK_DGRAM`) pour transporter nos propres paquets "TCP maison", ce qui nous permet de **contrôler et observer chaque étape**.

---

## 2. Architecture du projet

```
TCPhandshake/
├── common.hpp   ← Structures de données partagées (paquet, flags, affichage)
├── server.cpp   ← Programme serveur (écoute et répond au handshake)
├── client.cpp   ← Programme client (initie le handshake)
└── Makefile     ← Fichier de compilation
```

---

## 3. Les fichiers expliqués

### 3.1 `common.hpp`

C'est le fichier **partagé** entre le client et le serveur. Il définit tout ce dont les deux ont besoin.

#### Les flags TCP

```cpp
static constexpr uint8_t FLAG_FIN = 0x01;
static constexpr uint8_t FLAG_SYN = 0x02;
static constexpr uint8_t FLAG_RST = 0x04;
static constexpr uint8_t FLAG_ACK = 0x10;
```

Ces constantes représentent les **bits de contrôle** d'un vrai paquet TCP :

| Flag | Valeur | Rôle |
|------|--------|------|
| FIN  | 0x01   | Fermeture de connexion |
| SYN  | 0x02   | Synchronisation (démarrage de connexion) |
| RST  | 0x04   | Reset (fermeture brutale) |
| ACK  | 0x10   | Accusé de réception |

#### La structure `TCPPacket`

```cpp
struct TCPPacket {
    uint32_t seq;       // Numéro de séquence
    uint32_t ack;       // Numéro d'acquittement
    uint8_t  flags;     // Combinaison de flags (SYN, ACK, etc.)
    uint16_t data_len;  // Taille des données
    char     data[256]; // Données (payload)
};
```

C'est la structure de notre "faux paquet TCP". Elle contient :

- **`seq` (Sequence Number)** : un numéro qui identifie la position de ce paquet dans le flux. Chaque côté choisit un numéro de départ aléatoire appelé **ISN** (Initial Sequence Number).
- **`ack` (Acknowledgment Number)** : le numéro de séquence que l'on attend de recevoir de l'autre côté. Il vaut toujours `seq_reçu + 1`.
- **`flags`** : un octet dont chaque bit représente un flag (SYN, ACK, etc.). On peut combiner les flags avec l'opérateur `|` : ex. `FLAG_SYN | FLAG_ACK`.

#### Sérialisation réseau (`hton` / `ntoh`)

```cpp
void hton() {
    seq      = htonl(seq);   // host-to-network long
    ack      = htonl(ack);
    data_len = htons(data_len); // host-to-network short
}

void ntoh() {
    seq      = ntohl(seq);   // network-to-host long
    ack      = ntohl(ack);
    data_len = ntohs(data_len);
}
```

Les processeurs x86 stockent les entiers en **little-endian** (octet de poids faible en premier), mais le réseau utilise le **big-endian** (octet de poids fort en premier). Ces fonctions font la conversion avant d'envoyer (`hton`) et après avoir reçu (`ntoh`).

---

### 3.2 `server.cpp`

Le serveur joue le rôle de la machine qui **attend** une connexion. Il suit ces 3 étapes :

#### Étape 1 — Attendre le SYN

```cpp
recvfrom(fd, &pkt, sizeof(pkt), 0, &client, &clen);
pkt.ntoh(); // désérialiser
```

Le serveur bloque sur `recvfrom` jusqu'à recevoir un paquet. Il vérifie que le flag `SYN` est présent. Il récupère le `seq` du client (son ISN), qu'on appelle `client_isn`.

#### Étape 2 — Envoyer le SYN-ACK

```cpp
synack.flags = FLAG_SYN | FLAG_ACK;
synack.seq   = server_isn;       // ISN aléatoire du serveur
synack.ack   = client_isn + 1;  // on acquitte le SYN du client
```

Le serveur génère son propre ISN aléatoire (`server_isn`). Il envoie un paquet avec les flags `SYN` et `ACK` simultanément.

Le champ `ack = client_isn + 1` signifie : *"j'ai bien reçu ton SYN (qui compte pour 1 octet logique), le prochain octet que j'attends de toi porte le numéro `client_isn + 1`"*.

#### Étape 3 — Recevoir le ACK et valider

```cpp
if (ack.ack != server_isn + 1) {
    // erreur : le client n'a pas correctement acquitté notre SYN
}
```

Le serveur vérifie que le client a bien acquitté son SYN en contrôlant que `ack.ack == server_isn + 1`. Si tout est bon, la connexion est établie.

---

### 3.3 `client.cpp`

Le client initie la connexion. Il suit les mêmes 3 étapes mais dans le sens inverse :

#### Étape 1 — Envoyer le SYN

```cpp
syn.flags = FLAG_SYN;
syn.seq   = client_isn; // ISN aléatoire du client
syn.ack   = 0;          // pas encore d'acquittement
```

Le client génère son ISN aléatoire et envoie un SYN au serveur. Le champ `ack` est à 0 car on n'a encore rien reçu.

#### Étape 2 — Recevoir le SYN-ACK et valider

```cpp
if (synack.ack != client_isn + 1) {
    // erreur : le serveur n'a pas bien acquitté notre SYN
}
uint32_t server_isn = synack.seq; // on récupère l'ISN du serveur
```

Le client vérifie que le serveur a bien acquitté son SYN.

#### Étape 3 — Envoyer le ACK final

```cpp
ack.flags = FLAG_ACK;
ack.seq   = client_isn + 1;  // notre séquence avance
ack.ack   = server_isn + 1;  // on acquitte le SYN du serveur
```

Le client acquitte à son tour le SYN du serveur. Le handshake est terminé.

---

### 3.4 `Makefile`

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

all: server client

server: server.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o server server.cpp

client: client.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

clean:
	rm -f server client
```

Permet de compiler les deux programmes en une seule commande (`make`). Si `common.hpp` change, les deux programmes sont recompilés automatiquement.

---

## 4. Le déroulement complet du handshake

```
CLIENT                                    SERVER
  |                                          |
  |  1. SYN                                 |
  |  seq=X, ack=0, flags=SYN               |
  |─────────────────────────────────────>   |
  |                                          |
  |                          2. SYN-ACK     |
  |                  seq=Y, ack=X+1        |
  |                  flags=SYN|ACK         |
  |   <─────────────────────────────────────|
  |                                          |
  |  3. ACK                                 |
  |  seq=X+1, ack=Y+1, flags=ACK          |
  |─────────────────────────────────────>   |
  |                                          |
  |          CONNEXION ÉTABLIE              |
```

### Résumé des échanges

| Étape | Émetteur | Paquet envoyé | Signification |
|-------|----------|---------------|---------------|
| 1 | Client | `SYN, seq=X` | "Je veux me connecter, mon numéro de départ est X" |
| 2 | Serveur | `SYN-ACK, seq=Y, ack=X+1` | "OK, j'accepte. Mon numéro de départ est Y. J'ai reçu jusqu'à X." |
| 3 | Client | `ACK, seq=X+1, ack=Y+1` | "Compris. J'ai reçu jusqu'à Y. On peut commencer." |

---

## 5. Commandes pour lancer le projet

### Compiler

```bash
make
```

### Lancer en local (client et serveur sur la même machine)

Ouvrir **deux terminaux** :

**Terminal 1 — Démarrer le serveur :**
```bash
./server
```

**Terminal 2 — Démarrer le client :**
```bash
./client
```

Ou en une seule commande (serveur en arrière-plan) :
```bash
./server & sleep 0.2 && ./client
```

### Lancer sur deux machines différentes

**Machine A (serveur) :**
```bash
./server
```

**Machine B (client) — remplacer `IP_DU_SERVEUR` par l'IP réelle :**
```bash
./client IP_DU_SERVEUR
```

Exemple :
```bash
./client 192.168.1.42
```

### Nettoyer les binaires compilés

```bash
make clean
```

---

## 6. Exemple de sortie

**Côté client :**
```
=== CLIENT connecting to 127.0.0.1:8080 ===

[CLIENT --> SERVER] flags=SYN          seq=559109419  ack=0
[CLIENT <-- SERVER] flags=SYN ACK      seq=846732100  ack=559109420
[CLIENT --> SERVER] flags=ACK          seq=559109420  ack=846732101

=== Handshake COMPLETE — connexion établie ===
    client ISN: 559109419
    server ISN: 846732100
```

**Côté serveur :**
```
=== SERVER listening on port 8080 ===

[SERVER <-- CLIENT] flags=SYN          seq=559109419  ack=0
[SERVER --> CLIENT] flags=SYN ACK      seq=846732100  ack=559109420
[SERVER <-- CLIENT] flags=ACK          seq=559109420  ack=846732101

=== Handshake COMPLETE — connexion établie ===
    client ISN: 559109419
    server ISN: 846732100
    next expected from client: 559109420
```

---

## 7. Points techniques importants

### Pourquoi les numéros de séquence sont aléatoires ?

Dans le vrai TCP, l'ISN est semi-aléatoire pour des raisons de **sécurité**. Si les ISN étaient prévisibles (0, 1, 2...), un attaquant pourrait injecter de faux paquets en devinant le numéro attendu. L'aléatoire empêche ce type d'attaque (**TCP sequence number prediction attack**).

### Pourquoi SYN "consomme" 1 numéro de séquence ?

Dans TCP, le SYN et le FIN comptent chacun pour **1 octet logique** même s'ils ne transportent pas de données. C'est pourquoi `ack = seq_reçu + 1` après un SYN, et non `ack = seq_reçu`. Cela permet à l'autre côté de confirmer qu'il a reçu le signal de synchronisation.

### Byte order (endianness)

On appelle toujours `hton()` avant d'envoyer et `ntoh()` après avoir reçu. Si on oublie, les numéros de séquence sont interprétés à l'envers et la validation échoue sur une vraie connexion réseau (en local, les deux machines ayant le même endianness, l'erreur peut passer inaperçue).

### Transport via UDP

On utilise UDP (`SOCK_DGRAM`) comme "câble" pour transporter nos faux paquets TCP. UDP ne garantit pas la livraison ni l'ordre, ce qui est réaliste : le vrai TCP non plus ne peut pas supposer que le réseau est fiable, d'où les mécanismes de retransmission (non implémentés ici, car l'objectif est de comprendre le handshake uniquement).
