# Oral de présentation — TCP (Transmission Control Protocol)

---

## Introduction

Alors, imaginez que vous envoyez un colis à quelqu'un. Vous avez deux options : soit vous le glissez dans une boîte sans rien vérifier et vous croisez les doigts pour qu'il arrive, soit vous utilisez un service de livraison avec suivi, accusé de réception, et possibilité de renvoyer le colis s'il est perdu. TCP, c'est exactement la deuxième option — mais pour les données sur Internet.

TCP, ça veut dire **Transmission Control Protocol**. C'est un protocole de communication qui opère au niveau de la couche transport dans le modèle OSI. Il a été formalisé en 1981 dans le RFC 793 par Vint Cerf et Bob Kahn, et aujourd'hui il constitue encore la colonne vertébrale d'une grande partie d'Internet : HTTP, HTTPS, SSH, les emails, les transferts de fichiers... tout ça repose sur TCP.

La question centrale autour de TCP, c'est : *comment garantir qu'une donnée envoyée sur un réseau — qui est par nature peu fiable — arrive intacte, dans le bon ordre, et sans perte ?* C'est exactement ce problème que TCP résout.

---

## 1. Le contexte : pourquoi TCP existe

Pour comprendre TCP, il faut d'abord comprendre ce qu'est IP — Internet Protocol. IP est responsable du routage des paquets d'une machine à une autre. Mais IP est dit **non fiable** : il fait de son mieux pour acheminer les paquets, mais il n'a aucune garantie. Un paquet peut être perdu, dupliqué, ou arriver dans le désordre. IP s'en fiche.

C'est là qu'entre TCP. TCP s'appuie sur IP mais ajoute au-dessus une couche de fiabilité. Il garantit :

- **La livraison** : tout ce qui est envoyé finit par arriver, ou une erreur est levée.
- **L'ordre** : les données arrivent dans le même ordre qu'elles ont été envoyées.
- **L'intégrité** : via des checksums, on vérifie que les données n'ont pas été corrompues.
- **Le contrôle de flux** : on ne submerge pas le destinataire si il reçoit les données trop vite.

Son pendant, c'est UDP — User Datagram Protocol — qui lui est rapide mais sans aucune garantie. UDP c'est le "fire and forget" : on envoie et on n'attend rien en retour. UDP est utilisé pour la vidéo en streaming, les jeux en ligne, la VoIP, là où une légère perte est acceptable mais la latence doit être minimale. TCP, lui, est utilisé quand on veut de la fiabilité.

---

## 2. La connexion TCP : le Three-Way Handshake

Avant d'envoyer quoi que ce soit, TCP établit une connexion. Et cette connexion se fait en trois étapes. On appelle ça le **three-way handshake**, ou poignée de main en trois temps.

```
Client                          Serveur
  |                                |
  |---------- SYN (seq=x) -------->|   ① Le client demande une connexion
  |                                |
  |<----- SYN-ACK (seq=y, ack=x+1)-|   ② Le serveur acquitte et envoie son SYN
  |                                |
  |---------- ACK (ack=y+1) ------>|   ③ Le client acquitte le SYN du serveur
  |                                |
  |====== CONNEXION ÉTABLIE =======|
```

**Étape 1 — SYN** : Le client envoie un segment SYN (SYNchronize) au serveur. Il dit en substance : "Je veux ouvrir une connexion, et voici mon numéro de séquence initial : x."

**Étape 2 — SYN-ACK** : Le serveur répond avec un SYN-ACK. Il acquitte la demande du client (ack = x+1) et envoie son propre numéro de séquence y. Il dit : "D'accord, j'ai bien reçu ton SYN, et voilà le mien."

**Étape 3 — ACK** : Le client acquitte le SYN du serveur (ack = y+1). La connexion est établie des deux côtés.

Ces numéros de séquence sont fondamentaux — ils permettent à TCP de réordonner les segments qui arrivent dans le désordre, et de détecter les pertes.

---

## 3. Le transfert de données : segments, ACKs et retransmission

Une fois la connexion établie, TCP découpe les données en **segments**. Chaque segment est numéroté avec son numéro de séquence. Le destinataire, à chaque réception, renvoie un **ACK** — un accusé de réception — pour confirmer ce qu'il a bien reçu.

Si l'émetteur n'obtient pas d'ACK dans un certain délai, il **retransmet** le segment. C'est le mécanisme de base de la fiabilité TCP.

Mais envoyer un segment, attendre l'ACK, puis envoyer le suivant — ce serait terriblement lent. C'est pourquoi TCP utilise une **fenêtre glissante** (*sliding window*). L'émetteur peut envoyer plusieurs segments à la suite sans attendre d'ACK pour chacun, dans la limite d'une taille de fenêtre. Le destinataire acquitte les segments reçus, ce qui "glisse" la fenêtre et permet d'en envoyer de nouveaux.

Concrètement : si la fenêtre est de 3 segments, l'émetteur peut envoyer les segments 1, 2, 3. Quand il reçoit l'ACK du 1, il peut envoyer le 4, etc. Ça maximise l'utilisation du réseau.

---

## 4. Le contrôle de flux et de congestion

TCP gère deux choses distinctes mais liées : le **contrôle de flux** et le **contrôle de congestion**.

**Contrôle de flux** : il protège le destinataire. Si le récepteur est lent à traiter les données, il peut réduire la taille de sa fenêtre de réception pour dire à l'émetteur "ralentis, tu m'envoies trop vite". C'est une communication directe entre les deux endpoints.

**Contrôle de congestion** : il protège le réseau lui-même. Quand on commence à perdre des paquets, ça signifie que le réseau est saturé. TCP a plusieurs algorithmes pour gérer ça :

- **Slow Start** : au démarrage d'une connexion, on commence doucement et on augmente exponentiellement la fenêtre d'envoi jusqu'à un seuil.
- **Congestion Avoidance** : une fois le seuil atteint, on augmente plus lentement, de manière linéaire.
- **Fast Retransmit / Fast Recovery** : si on reçoit trois ACKs dupliqués pour le même segment, on déduit une perte et on retransmet immédiatement sans attendre le timeout.

Ces mécanismes font que TCP s'adapte automatiquement aux conditions du réseau — un réseau chargé va ralentir l'émetteur, un réseau libre va lui permettre d'accélérer.

---

## 5. La fermeture de connexion : le Four-Way Handshake

La fermeture d'une connexion TCP est un peu plus complexe que l'ouverture. Elle se fait en **quatre étapes** — le four-way handshake — parce que les deux sens de communication sont indépendants et doivent être fermés séparément.

```
Client                          Serveur
  |                                |
  |------------ FIN -------------->|   ① Le client ferme son sens d'envoi
  |<----------- ACK ---------------|   ② Le serveur acquitte
  |                                |
  |<----------- FIN ---------------|   ③ Le serveur ferme son sens d'envoi
  |------------ ACK -------------->|   ④ Le client acquitte
  |                                |
  | [TIME_WAIT ~2×MSL]             |
  |                                |
  |======= CONNEXION FERMÉE =======|
```

Le client envoie un **FIN** pour dire qu'il n'a plus rien à envoyer. Le serveur l'acquitte avec un **ACK**. Puis le serveur, quand il a fini d'envoyer ses propres données, envoie son propre **FIN**. Le client l'acquitte avec un dernier **ACK**.

Il y a ensuite un état appelé **TIME_WAIT** du côté du client, qui dure typiquement 2 × le maximum segment lifetime — pour s'assurer que tous les paquets résiduels sur le réseau ont eu le temps d'expirer avant de fermer définitivement la connexion.

---

## 6. TCP dans le modèle OSI et comparaison UDP

TCP opère à la **couche 4 — Transport** du modèle OSI. Au-dessus de lui se trouvent les protocoles applicatifs comme HTTP, FTP, SMTP. En dessous, IP s'occupe du routage.

| | TCP | UDP |
|---|---|---|
| Connexion | Oui (handshake) | Non |
| Fiabilité | Garantie | Non garantie |
| Ordre | Garanti | Non garanti |
| Débit | Plus faible | Plus élevé |
| Usage | Web, emails, SSH | Streaming, DNS, jeux |

---

## Conclusion

TCP est un protocole vieux de plus de 40 ans, et pourtant il reste omniprésent. Sa force, c'est qu'il résout un problème fondamental — la fiabilité sur un réseau peu fiable — avec des mécanismes élégants : numéros de séquence, fenêtre glissante, retransmission automatique, contrôle de congestion.

Avec HTTP/3, on voit émerger QUIC — un protocole qui implémente des fonctionnalités similaires à TCP mais directement sur UDP, pour gagner en latence. Mais TCP reste le standard de référence, et comprendre TCP, c'est comprendre les fondations sur lesquelles repose une grande partie d'Internet.

---
