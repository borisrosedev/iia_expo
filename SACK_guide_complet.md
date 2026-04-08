# SACK — Selective Acknowledgment
### Tout comprendre sur l'acquittement sélectif en TCP

---

## Table des matières

1. [Les bases de TCP](#1-les-bases-de-tcp)
2. [Les ACK et le problème cumulatif](#2-les-ack-et-le-problème-cumulatif)
3. [SACK — la solution](#3-sack--la-solution)
4. [Format technique](#4-format-technique)
5. [D-SACK — l'extension](#5-d-sack--lextension)
6. [Impact sur les performances](#6-impact-sur-les-performances)
7. [Limites et vulnérabilités](#7-limites-et-vulnérabilités)
8. [SACK aujourd'hui](#8-sack-aujourdhui)
9. [Glossaire](#9-glossaire)

---

## 1. Les bases de TCP

TCP — *Transmission Control Protocol* — est l'un des protocoles fondamentaux d'internet. Il opère au niveau de la **couche transport** du modèle OSI, juste au-dessus d'IP.

Son rôle est de fournir une communication **fiable, ordonnée et sans erreur** entre deux machines. Concrètement, ça veut dire :

- Les données arrivent **dans le bon ordre**, même si les paquets ont pris des chemins différents sur le réseau.
- Les données arrivent **en intégralité**, même si certains paquets se sont perdus en route.
- Les données arrivent **sans corruption**, grâce à des sommes de contrôle.

Pour garantir tout ça, TCP utilise plusieurs mécanismes : la numérotation des segments, les accusés de réception, les retransmissions, et le contrôle de congestion.

### La fenêtre d'envoi

TCP n'envoie pas les données segment par segment en attendant une confirmation à chaque fois — ce serait beaucoup trop lent. Il envoie plusieurs segments d'affilée sans attendre, dans ce qu'on appelle une **fenêtre d'envoi** (*congestion window*). La taille de cette fenêtre détermine combien de segments peuvent être "en vol" simultanément, c'est-à-dire envoyés mais pas encore confirmés.

Plus la fenêtre est grande, plus le débit potentiel est élevé. Mais plus il y a de segments en vol, plus les conséquences d'une perte sont importantes.

### Le handshake TCP

Avant tout échange de données, TCP établit une connexion via un **handshake en trois étapes** :

1. **SYN** — le client envoie une demande de connexion.
2. **SYN-ACK** — le serveur accepte et répond.
3. **ACK** — le client confirme, la connexion est établie.

C'est pendant ce handshake que les deux machines s'échangent leurs capacités et options supportées, dont SACK.

---

## 2. Les ACK et le problème cumulatif

### Comment fonctionnent les ACK

Un **ACK** (*Acknowledgment*) est un message envoyé par le récepteur pour confirmer la bonne réception de données. Dans TCP, chaque byte de données est numéroté grâce à un **numéro de séquence**. Un ACK indique le numéro du prochain byte attendu.

Par exemple, si le récepteur envoie `ACK = 1001`, ça signifie : *"j'ai bien reçu tout jusqu'au byte 1000, envoie-moi la suite à partir de 1001."*

Ce système est dit **cumulatif** : un seul ACK confirme implicitement tout ce qui précède. C'est simple et économique en bande passante.

### Le problème : que se passe-t-il quand un segment se perd ?

Imaginons que l'émetteur envoie 8 segments numérotés de 1 à 8, et que le segment 3 se perd en route. Le récepteur reçoit 1, 2, puis 4, 5, 6, 7, 8.

Le récepteur est bloqué. TCP garantit la livraison **ordonnée**, il ne peut donc pas confirmer les segments 4 à 8 tant que le 3 n'est pas arrivé. Il met les segments reçus de côté dans un buffer, et répète inlassablement le même ACK : `ACK = 3` — *"j'attends toujours le segment 3"*.

Quand l'émetteur reçoit **trois ACK identiques consécutifs** — ce qu'on appelle un *triple duplicate ACK* — il comprend qu'un segment est perdu et déclenche la retransmission.

### Le Go-Back-N implicite

Le vrai problème vient de ce que TCP classique retransmet alors. Ne sachant pas ce que le récepteur a déjà reçu au-delà du trou, il doit retransmettre **tout depuis le segment manquant**. C'est le principe du **Go-Back-N** : retourner en arrière au premier segment non confirmé et reprendre depuis là.

Dans notre exemple, l'émetteur retransmet 3, 4, 5, 6, 7 et 8 — alors que 4 à 8 étaient déjà arrivés. Six segments retransmis inutilement pour une seule perte.

Sur une petite fenêtre, c'est acceptable. Sur une fenêtre de 100 ou 200 segments — ce qui est courant sur les réseaux modernes à haut débit ou à longue latence — c'est catastrophique.

---

## 3. SACK — la solution

### Le principe

SACK, défini dans la **RFC 2018** publiée en octobre 1996, apporte une solution élégante : donner au récepteur la capacité de décrire précisément ce qu'il a reçu, y compris les plages non contiguës.

Pour ça, SACK ajoute une **option dans l'en-tête TCP** qui liste des **blocs de segments contigus** déjà reçus. Un bloc SACK, c'est une paire de numéros de séquence : le début et la fin d'une plage reçue.

### Un exemple concret

En reprenant notre scénario avec le segment 3 perdu, la réponse SACK du récepteur devient :

```
ACK = 3
SACK block : [4, 9]
```

Ce qui se lit : *"j'attends toujours le segment 3, et j'ai déjà reçu les segments 4 à 8."*

L'émetteur sait désormais exactement quoi faire : retransmettre **uniquement le segment 3**. Rien d'autre.

### La scoreboard

Côté émetteur, SACK implique la gestion d'une **scoreboard** — un tableau de bord interne qui enregistre, pour chaque segment envoyé, s'il a été confirmé, s'il est signalé comme reçu via SACK, ou s'il est considéré comme perdu.

C'est cette scoreboard qui permet à l'émetteur de savoir précisément quels segments retransmettre, et dans quel ordre.

### Du Go-Back-N au Selective Repeat

Avec SACK, TCP passe d'une stratégie **Go-Back-N** à une stratégie **Selective Repeat** :

| | Go-Back-N (TCP classique) | Selective Repeat (TCP + SACK) |
|---|---|---|
| En cas de perte | Retransmet tout depuis le premier trou | Retransmet uniquement les segments perdus |
| Efficacité | Faible si fenêtre grande | Optimale |
| Complexité émetteur | Simple | Plus complexe |
| Complexité récepteur | Simple | Buffer requis |

---

## 4. Format technique

### Négociation au handshake

SACK ne s'active pas automatiquement. Il doit être **négocié** lors du handshake TCP. Chaque machine annonce dans ses options TCP l'option `SACK-permitted`. Si les deux hôtes la supportent, SACK est actif pour toute la connexion. Sinon, la connexion fonctionne normalement en TCP classique.

Cette rétrocompatibilité est fondamentale : SACK ne casse rien et ne nécessite aucune configuration manuelle.

### Structure d'un bloc SACK

Un bloc SACK est encodé sur **8 octets** :

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  Left Edge of Block (32 bits)                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  Right Edge of Block (32 bits)                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

- **Left Edge** : numéro de séquence du premier byte du bloc reçu.
- **Right Edge** : numéro de séquence du byte *suivant* le dernier byte du bloc (convention exclusive, comme en Python).

### Nombre de blocs par segment

La zone Options d'un en-tête TCP est limitée à **40 octets**. En tenant compte de 2 octets d'en-tête pour l'option SACK elle-même, il reste 38 octets — soit **4 blocs SACK maximum** (4 × 8 = 32 octets + 2 = 34 octets).

En pratique, quand l'option Timestamps (RFC 1323) est également active — ce qui est très courant — elle occupe 10 octets supplémentaires, réduisant le nombre de blocs SACK à **3 maximum**.

### Ordre des blocs

La RFC 2018 recommande de placer en premier le bloc le plus récemment reçu (*most recently received*). Ça permet à l'émetteur de détecter rapidement les nouvelles pertes, même si la limite de 4 blocs est atteinte.

---

## 5. D-SACK — l'extension

### RFC 2883

**D-SACK** (*Duplicate SACK*), défini dans la RFC 2883 en juillet 2000, est une extension de SACK qui permet au récepteur de signaler qu'il a reçu un segment **en double**.

### À quoi ça sert ?

Sans D-SACK, l'émetteur ne peut pas distinguer deux situations :
1. Le segment s'est perdu → retransmission nécessaire.
2. Le segment est arrivé mais l'ACK s'est perdu → retransmission inutile.

Avec D-SACK, le récepteur peut dire : *"j'ai reçu ce segment deux fois"*. L'émetteur comprend que le segment était en réalité arrivé, et que c'est l'ACK qui était en retard ou perdu.

### Conséquences pratiques

Cette information est précieuse pour les algorithmes de contrôle de congestion. Si les pertes détectées sont en réalité des faux positifs causés par des ACK retardés, TCP n'a pas de raison de réduire son débit. D-SACK permet donc d'**éviter des ralentissements inutiles**.

D-SACK aide également à calibrer le **RTO** (*Retransmission Timeout*) — le délai après lequel TCP considère qu'un segment est perdu s'il n'a pas reçu d'ACK.

---

## 6. Impact sur les performances

### Le RTT comme unité de mesure

La métrique clé pour évaluer l'impact de SACK est le **RTT** — *Round Trip Time* — le temps d'un aller-retour entre émetteur et récepteur.

Sans SACK, chaque segment perdu nécessite au minimum un RTT pour être détecté et retransmis. Avec N pertes dans une fenêtre, on peut avoir besoin de N RTT successifs pour tout récupérer.

Avec SACK, **un seul RTT suffit** : le récepteur signale tous les trous en une seule réponse, l'émetteur les comble tous en une seule rafale.

### Le BDP — Bandwidth-Delay Product

Le **BDP** (*Bandwidth-Delay Product*) est la quantité de données pouvant être "en vol" simultanément sur un lien. Il se calcule simplement :

```
BDP = Débit (bits/s) × RTT (s)
```

Sur un lien satellite à 100 Mbps avec 600ms de RTT :

```
BDP = 100 000 000 × 0.6 = 60 000 000 bits = 7,5 Mo
```

Ça représente des milliers de segments TCP en transit en même temps. Si 1% se perdent, ça fait des dizaines de segments perdus par fenêtre. Sans SACK, récupérer tous ces segments peut prendre de nombreux RTT — soit plusieurs secondes sur un lien satellite. Avec SACK, c'est un seul RTT.

### Cas d'usage où SACK est critique

**Liaisons satellite** : RTT de 500 à 700ms, fenêtres TCP énormes du fait du BDP. Chaque RTT supplémentaire coûte une demi-seconde. SACK est quasiment indispensable pour atteindre un débit correct.

**Réseaux Wi-Fi et mobiles** : ces médias ont des taux de perte naturels de 1 à 5% dus aux interférences radio. Ces pertes ne signalent pas de congestion, mais TCP classique les interprète comme tel et réduit son débit inutilement. SACK + D-SACK permettent de récupérer efficacement sans sur-réagir.

**Transferts massifs** : lors d'un transfert de fichier volumineux, la fenêtre TCP peut contenir des centaines de segments. SACK garantit qu'une perte isolée ne provoque pas la retransmission de tout ce qui suit.

---

## 7. Limites et vulnérabilités

### SACK spoofing

Un récepteur malveillant pourrait mentir dans ses blocs SACK — annoncer des plages de segments reçus qui ne correspondent pas à la réalité. L'objectif peut être de forcer l'émetteur à des retransmissions inutiles pour consommer ses ressources (*attaque par épuisement*), ou de manipuler la fenêtre de congestion.

Les implémentations modernes intègrent des vérifications pour limiter ce risque.

### SACK Panic — CVE-2019-11477

En juin 2019, une vulnérabilité critique a été découverte dans le noyau Linux, baptisée **SACK Panic**. En envoyant une séquence de segments SACK malformés avec une taille maximale de segment (MSS) très basse, un attaquant pouvait provoquer un dépassement d'entier dans le code de gestion de SACK, entraînant un **kernel panic** — un crash complet du système.

Cette vulnérabilité (et plusieurs autres connexes découvertes en même temps) a été corrigée rapidement dans les noyaux Linux concernés. Elle illustre la complexité d'implémenter correctement SACK.

### La contrainte des 4 blocs

Dans des scénarios de pertes très fragmentées — de nombreux segments perdus de manière non contiguë — la limite de 4 blocs SACK peut s'avérer insuffisante pour décrire tous les trous en une seule réponse. L'émetteur devra traiter les pertes en plusieurs passes, nécessitant plusieurs RTT.

Ce cas reste rare sur les réseaux courants, mais peut survenir lors d'attaques ou de conditions réseau dégradées.

### Complexité d'implémentation

Gérer correctement SACK côté émetteur est significativement plus complexe que TCP classique. Il faut maintenir la scoreboard, gérer les interactions entre SACK et les algorithmes de contrôle de congestion (CUBIC, BBR, RENO…), et traiter correctement les cas limites (retransmissions partielles, fenêtre qui se réduit pendant une récupération SACK, etc.).

Des bugs ont existé dans des implémentations passées, parfois causant des performances sous-optimales ou des comportements incorrects dans des conditions spécifiques.

---

## 8. SACK aujourd'hui

### Déploiement

SACK est aujourd'hui activé **par défaut** sur les principaux systèmes d'exploitation :

- **Linux** : depuis le noyau 2.4 (2001). Vérifiable avec `sysctl net.ipv4.tcp_sack`.
- **Windows** : depuis Windows XP.
- **macOS / iOS** : depuis les premières versions OS X.
- **FreeBSD, OpenBSD** : supporté de longue date.

On estime que SACK est présent dans la quasi-totalité des connexions TCP sur internet aujourd'hui.

### Relation avec QUIC

QUIC est un protocole de transport moderne développé par Google et standardisé par l'IETF (RFC 9000, 2021). Il intègre nativement des mécanismes similaires à SACK — les **ACK ranges** — qui permettent de signaler des plages de paquets reçus avec une granularité encore plus fine que SACK.

QUIC fonctionne sur UDP et intègre également le chiffrement TLS 1.3 nativement, ce qui le rend plus difficile à optimiser pour les équipements réseau intermédiaires. TCP + SACK reste donc très largement dominant pour la grande majorité des connexions internet.

### Vérifier SACK sur Linux

```bash
# Vérifier si SACK est activé
sysctl net.ipv4.tcp_sack
# net.ipv4.tcp_sack = 1  → activé

# Activer manuellement si nécessaire
sysctl -w net.ipv4.tcp_sack=1

# Observer les statistiques SACK sur une connexion avec ss
ss -tin dst <adresse_ip>
# Chercher les champs : sacked, retrans, lost
```

---

## 9. Glossaire

| Terme | Définition |
|-------|-----------|
| **ACK** | *Acknowledgment* — accusé de réception envoyé par le récepteur pour confirmer la réception de données. |
| **ACK cumulatif** | ACK qui confirme implicitement la réception de tout ce qui précède un certain numéro de séquence. |
| **BDP** | *Bandwidth-Delay Product* — quantité de données pouvant être en vol simultanément sur un lien (débit × RTT). |
| **Bloc SACK** | Plage de segments contigus reçus, encodée sur 8 octets (numéro de début + numéro de fin). |
| **Buffer** | Zone mémoire où le récepteur stocke temporairement les segments reçus en avance, en attendant que les trous soient comblés. |
| **Congestion window** | Fenêtre de congestion — nombre maximal de segments que l'émetteur peut avoir "en vol" sans avoir reçu d'ACK. |
| **D-SACK** | *Duplicate SACK* — extension (RFC 2883) permettant de signaler la réception d'un segment en double. |
| **Go-Back-N** | Stratégie de retransmission où l'émetteur retransmet tout depuis le premier segment non confirmé. |
| **Handshake** | Poignée de mains TCP en trois étapes (SYN / SYN-ACK / ACK) établissant la connexion. |
| **MSS** | *Maximum Segment Size* — taille maximale des données dans un segment TCP, négociée au handshake. |
| **RFC** | *Request for Comments* — document de standardisation publié par l'IETF définissant les protocoles internet. |
| **RTO** | *Retransmission Timeout* — délai après lequel TCP retransmet un segment s'il n'a pas reçu d'ACK. |
| **RTT** | *Round Trip Time* — temps d'un aller-retour entre émetteur et récepteur. |
| **SACK** | *Selective Acknowledgment* — mécanisme TCP (RFC 2018) permettant de signaler des plages non contiguës de segments reçus. |
| **Scoreboard** | Tableau de bord interne à l'émetteur listant l'état de chaque segment envoyé (reçu, SACK, perdu). |
| **Selective Repeat** | Stratégie de retransmission où l'émetteur retransmet uniquement les segments effectivement perdus. |
| **Segment** | Unité de données TCP — bloc de bytes numérotés envoyé en une seule unité. |
| **Slow start** | Phase initiale de TCP où la fenêtre de congestion croît exponentiellement jusqu'à détection d'une perte. |
| **TCP** | *Transmission Control Protocol* — protocole de transport fiable, ordonné et sans perte défini dans la RFC 793. |
| **Triple duplicate ACK** | Trois ACK identiques consécutifs reçus par l'émetteur, signal qu'un segment est probablement perdu. |

---

*Sources : RFC 2018 (SACK), RFC 2883 (D-SACK), RFC 793 (TCP), RFC 9000 (QUIC), CVE-2019-11477.*
