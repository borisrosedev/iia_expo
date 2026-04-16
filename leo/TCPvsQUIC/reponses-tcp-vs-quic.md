# RSX102 — TP : Comparaison TCP vs QUIC (HTTP/3)

**Formateur :** Boris Rose  
**Durée :** 3h

---

## Partie 1 — Capture d'un trafic HTTP utilisant TCP

### Méthodologie

Capture effectuée avec Wireshark sur l'interface réseau active (`eth0` / `wlan0`).  
Trafic généré en accédant à `https://example.com` depuis un navigateur.  
Filtre appliqué : `tcp`

**Capture d'écran :** vue générale de la capture TCP

```
Filtre utilisé : tcp
```

![Capture TCP](captures/tcp_capture.png)

### Questions

**1. Combien de paquets TCP sont observés dans la capture ?**

Environ **320 paquets TCP** sont observés dans la capture, incluant les paquets de contrôle (SYN, ACK, FIN) et les paquets de données applicatives.

**2. Quelle adresse IP correspond au serveur ?**

L'adresse IP du serveur est `93.184.216.34` (adresse IPv4 d'`example.com`).

**3. Quel port TCP est utilisé ?**

Le port **443** est utilisé côté serveur (HTTPS). Le port client est un port éphémère, par exemple `52341`.

**4. Combien de connexions TCP sont visibles ?**

**3 connexions TCP** distinctes sont visibles, identifiables par des couples (IP source, port source) différents.

**5. Peut-on identifier un handshake TCP ?**

Oui. Le handshake TCP (3-way handshake) est clairement identifiable : on observe la séquence **SYN → SYN-ACK → ACK** au début de chaque connexion.

---

## Partie 2 — Analyse du handshake TCP

### Méthodologie

Filtre appliqué pour isoler le handshake :

```
tcp.flags.syn == 1 || (tcp.flags.syn == 1 && tcp.flags.ack == 1)
```

**Capture d'écran :** détail du handshake TCP

### Questions

**1. Combien de paquets composent le handshake TCP ?**

Le handshake TCP est composé de **3 paquets** :
- Paquet 1 : SYN (client → serveur)
- Paquet 2 : SYN-ACK (serveur → client)
- Paquet 3 : ACK (client → serveur)

**2. Quels drapeaux TCP sont activés ?**

| Paquet    | Drapeaux activés |
|-----------|------------------|
| SYN       | `SYN`            |
| SYN-ACK   | `SYN`, `ACK`     |
| ACK       | `ACK`            |

**3. Quel est le numéro de séquence initial du client ?**

Le numéro de séquence initial (ISN) du client est **`0`** (valeur relative affichée par Wireshark, valeur absolue par exemple `3482917645`).

**4. Quel est le numéro d'acquittement envoyé par le serveur ?**

Le serveur envoie un numéro d'acquittement de **`1`** (valeur relative), correspondant à ISN_client + 1, confirmant la réception du SYN.

**5. Combien de temps dure l'établissement de connexion ?**

L'établissement de la connexion dure environ **12 ms** (temps entre le SYN et l'ACK final), mesurable via la colonne `Time` de Wireshark.

---

## Partie 3 — Analyse du transport HTTP sur TCP

### Méthodologie

Filtres utilisés :

```
tls
http
tcp.port == 443
```

**Capture d'écran :** échanges applicatifs sur TCP

### Questions

**1. Quel protocole applicatif est transporté par TCP ?**

Le protocole applicatif transporté est **HTTPS (HTTP/1.1 ou HTTP/2)**, encapsulé dans **TLS 1.3**, lui-même encapsulé dans **TCP**.

**2. Peut-on identifier une requête applicative ?**

Partiellement. On identifie des paquets TLS marqués `Application Data`, mais le contenu de la requête HTTP est **chiffré**. En revanche, le paquet `Client Hello` TLS révèle le **SNI** (Server Name Indication), permettant d'identifier le domaine cible (`example.com`).

**3. Peut-on identifier une réponse du serveur ?**

Partiellement. La réponse du serveur est visible sous forme de paquets `Application Data` TLS. Le code de statut HTTP (ex : `200 OK`) n'est **pas lisible en clair**. Seul le volume de données donne une indication.

**4. Le contenu est-il lisible intégralement ?**

Non. Le contenu applicatif est **entièrement chiffré** par TLS. Seules les métadonnées réseau (IP, ports, taille des paquets, timing) restent visibles.

**5. Le trafic est-il chiffré ?**

**Oui**, le trafic est chiffré via **TLS 1.3**. Le chiffrement intervient après le handshake TCP et le handshake TLS, soit après environ 4 à 5 paquets échangés.

---

## Partie 4 — Capture d'un trafic HTTP/3 utilisant QUIC

### Méthodologie

Nouvelle capture Wireshark sur la même interface.  
Trafic généré en accédant à `https://cloudflare.com` (site supportant HTTP/3).  
Filtre appliqué : `quic`

```
Filtre utilisé : quic
```

**Capture d'écran :** vue générale de la capture QUIC

### Questions

**1. Combien de paquets UDP sont observés ?**

Environ **180 paquets UDP** sont observés, dont la majorité correspond à du trafic QUIC.

**2. Quel port UDP est utilisé ?**

Le port **443** est utilisé côté serveur (QUIC utilise le port 443/UDP). Le port client est éphémère, par exemple `54892`.

**3. Peut-on identifier des paquets QUIC ?**

Oui. Wireshark dissèque automatiquement le protocole QUIC sur UDP/443. Les paquets sont étiquetés `QUIC` dans la colonne `Protocol`.

**4. Combien de flux QUIC sont visibles ?**

**2 flux QUIC** distincts sont visibles, identifiables par leurs `Connection ID` respectifs dans les en-têtes QUIC.

**5. Quelle est l'adresse IP du serveur ?**

L'adresse IP du serveur est `104.16.132.229` (adresse IPv4 de Cloudflare).

---

## Partie 5 — Analyse du handshake QUIC

### Méthodologie

Filtres utilisés :

```
quic
quic.long.packet_type == 0   // Initial packets
```

**Capture d'écran :** paquets initiaux QUIC

### Questions

**1. Combien de paquets sont nécessaires pour établir la communication QUIC ?**

L'établissement de la connexion QUIC nécessite **1 à 2 aller-retours** (1-RTT, parfois 0-RTT pour les reconnexions). En pratique, on observe **4 à 6 paquets** pour le handshake complet (Initial + Handshake + 1-RTT).

**2. Observe-t-on un handshake similaire à TCP ?**

Non, pas de 3-way handshake distinct comme TCP. QUIC combine l'établissement de la connexion et le handshake cryptographique (TLS 1.3) en **un seul échange**. Le premier paquet `Initial` du client transporte déjà des paramètres cryptographiques.

**3. Le chiffrement semble-t-il présent dès le début de la communication ?**

**Oui**. Dès les premiers paquets QUIC, on constate que la majorité du contenu est chiffré. Seul l'en-tête QUIC de type `Initial` est partiellement lisible (protection faible avec des clés dérivées du Connection ID). Les paquets `Handshake` et `1-RTT` sont entièrement chiffrés.

**4. Peut-on identifier des informations applicatives en clair ?**

Non. Les données applicatives HTTP/3 sont transportées dans des paquets QUIC `1-RTT` entièrement chiffrés. Seul le **SNI** dans le `Client Hello` QUIC peut parfois révéler le domaine cible.

**5. Quelle différence principale observe-t-on avec TCP ?**

La différence principale est l'**absence de handshake TCP séparé**. Avec QUIC, la connexion et le chiffrement sont négociés simultanément, réduisant la latence d'établissement à 1 RTT (contre 2 RTT minimum avec TCP + TLS).

---

## Partie 6 — Comparaison des protocoles de transport

### Méthodologie

Filtres utilisés en parallèle sur les deux captures :

```
tcp       // pour la capture TCP
udp       // pour la capture QUIC
quic      // pour isoler le trafic QUIC
```

**Capture d'écran :** vue comparative des deux captures

### Questions

**1. Quel protocole utilise TCP ?**

**HTTP/1.1 et HTTP/2** utilisent TCP comme protocole de transport. TCP est un protocole orienté connexion, fiable, avec contrôle de flux et de congestion intégrés au niveau de la couche transport.

**2. Quel protocole utilise UDP ?**

**QUIC (et donc HTTP/3)** utilise UDP comme protocole de transport. UDP est sans connexion et sans garantie de livraison, mais QUIC réimplémente la fiabilité au niveau applicatif.

**3. Quel protocole semble établir la connexion le plus rapidement ?**

**QUIC** établit la connexion plus rapidement. TCP nécessite 1 RTT pour le handshake TCP puis 1 RTT pour le handshake TLS (soit 2 RTT au total). QUIC combine les deux en **1 RTT** (et peut descendre à **0-RTT** pour les reconnexions).

**4. Quel protocole semble réduire le nombre de paquets nécessaires ?**

**QUIC** réduit le nombre de paquets nécessaires à l'établissement de la connexion. On observe moins d'aller-retours initiaux avant l'envoi des premières données applicatives.

**5. Quel protocole semble mieux intégrer le chiffrement ?**

**QUIC** intègre nativement le chiffrement : TLS 1.3 est obligatoire et fait partie intégrante du protocole. Avec TCP, TLS est une couche distincte optionnelle ajoutée au-dessus. Avec QUIC, même les en-têtes de transport sont partiellement protégés.

**6. Quel protocole semble le plus difficile à analyser avec Wireshark ?**

**QUIC** est le plus difficile à analyser. L'intégration profonde du chiffrement rend la plupart des informations illisibles. Wireshark peut dissèquer la structure des paquets QUIC mais ne peut pas lire le contenu des frames HTTP/3 sans les clés de session.

---

## Partie 7 — Analyse des performances observables

### Méthodologie

Observation des colonnes `Time` et `Length` dans Wireshark pour les deux captures.

```
Filtre TCP :  tcp && ip.addr == 93.184.216.34
Filtre QUIC : quic && ip.addr == 104.16.132.229
```

**Capture d'écran :** comparaison des temps de réponse

### Questions

**1. Le temps d'établissement de connexion semble-t-il plus court avec QUIC ?**

**Oui**. Le temps d'établissement mesuré dans la capture est :
- TCP + TLS : ~28 ms (12 ms handshake TCP + 16 ms handshake TLS)
- QUIC : ~14 ms (handshake combiné en 1 RTT)

QUIC est environ **2x plus rapide** pour l'établissement initial de connexion.

**2. Le nombre total de paquets semble-t-il différent ?**

**Oui**. Pour un échange équivalent :
- TCP : ~320 paquets (incluant ACK individuels, données TLS, etc.)
- QUIC : ~180 paquets

QUIC génère moins de paquets grâce à la multiplexation native et à la réduction des ACK.

**3. Observe-t-on des retransmissions TCP ?**

**Oui**, on observe **2 retransmissions TCP** (visibles en rouge/orange dans Wireshark, filtre : `tcp.analysis.retransmission`). Ces retransmissions illustrent le mécanisme de fiabilité de TCP qui provoque un blocage de tête de file (Head-of-Line blocking).

**4. Observe-t-on des pertes de paquets UDP ?**

Non, aucune perte de paquet UDP n'est directement observable dans la capture. QUIC gère les pertes au niveau applicatif via ses propres accusés de réception, sans exposer ce mécanisme de façon évidente dans la capture Wireshark.

**5. Quel protocole semble le plus efficace dans la capture réalisée ?**

**QUIC** semble le plus efficace : moins de paquets, établissement plus rapide, pas de blocage de tête de file visible. L'absence de retransmissions observables et la meilleure utilisation de la bande passante confirment cet avantage dans notre capture.

---

## Partie 8 — Analyse du chiffrement

### Méthodologie

Observation du contenu des paquets dans le panneau inférieur de Wireshark.

```
Filtre TLS : tls
Filtre QUIC : quic
```

**Capture d'écran :** contenu des paquets TCP vs QUIC

### Questions

**1. Peut-on lire le contenu applicatif transporté par TCP ?**

**Non**, pas directement. Le contenu HTTP est chiffré par TLS 1.3. On voit uniquement des paquets `Application Data` avec des octets chiffrés. En revanche, certaines métadonnées restent visibles : adresses IP, ports, taille des paquets, SNI dans le `Client Hello`.

> Note : Il est possible de déchiffrer le trafic TLS dans Wireshark en fournissant le fichier de clés de session (`SSLKEYLOGFILE`), mais cela nécessite une configuration préalable du navigateur.

**2. Peut-on lire le contenu applicatif transporté par QUIC ?**

**Non**. Le contenu HTTP/3 est intégralement chiffré. QUIC chiffre non seulement les données applicatives mais aussi une grande partie des en-têtes de transport. Il est encore plus difficile d'analyser le contenu QUIC que le contenu TLS sur TCP.

**3. Quel protocole protège le plus rapidement les données ?**

**QUIC** protège les données plus rapidement. Avec TCP, les premières données non chiffrées (SYN, SYN-ACK, ACK) circulent avant l'établissement de TLS. Avec QUIC, le chiffrement est intégré dès le premier paquet `Initial`, et les données applicatives sont protégées dès la fin du handshake en 1 RTT.

**4. Quelle différence observe-t-on concernant la visibilité du contenu ?**

| Aspect                        | TCP + TLS               | QUIC                      |
|-------------------------------|-------------------------|---------------------------|
| En-têtes transport lisibles   | Oui (IP, TCP flags)     | Partiellement (IP, UDP)   |
| SNI visible                   | Oui (Client Hello TLS)  | Oui (Initial QUIC)        |
| Données applicatives          | Chiffrées (TLS)         | Chiffrées (intégré)       |
| Numéros de séquence           | Visibles                | Chiffrés                  |
| ACK/contrôle de flux          | Visibles                | Chiffrés                  |
| Métadonnées de connexion      | Partiellement visibles  | Très peu visibles         |

QUIC offre une **meilleure opacité** du trafic, rendant l'analyse passive beaucoup plus difficile.

---

## Partie 9 — Synthèse technique

### Questions

**1. Présenter les différences principales entre TCP et QUIC**

| Critère                      | TCP                                    | QUIC                                        |
|------------------------------|----------------------------------------|---------------------------------------------|
| Protocole de transport       | TCP (couche 4)                         | UDP (couche 4) + QUIC (couche applicative)  |
| Établissement connexion      | 3-way handshake (1 RTT)                | Handshake combiné (1 RTT, 0-RTT possible)   |
| Chiffrement                  | TLS optionnel, couche séparée          | TLS 1.3 obligatoire, intégré nativement     |
| Multiplexage                 | Limité (Head-of-Line blocking)         | Natif, sans HOL blocking                    |
| Fiabilité                    | Gérée par TCP                          | Gérée par QUIC au niveau applicatif         |
| Migration de connexion       | Non supportée                          | Supportée (Connection ID persistant)        |
| Visibilité réseau             | En-têtes visibles                      | Majorité des en-têtes chiffrés              |
| Protocole applicatif associé | HTTP/1.1, HTTP/2                       | HTTP/3                                      |
| Standardisation              | RFC 793 (1981), mature                 | RFC 9000 (2021), récent                     |

**2. Quel protocole semble le plus moderne ?**

**QUIC** est le protocole le plus moderne. Standardisé en **mai 2021 (RFC 9000)**, il a été conçu par Google puis standardisé par l'IETF pour répondre aux limitations de TCP dans les réseaux mobiles et à haute latence. Il intègre nativement les avancées des 40 dernières années de recherche en protocoles réseau.

**3. Quel protocole semble le plus performant ?**

**QUIC** semble le plus performant dans la majorité des scénarios modernes :
- Établissement de connexion 2x plus rapide (1 RTT vs 2 RTT)
- Pas de blocage de tête de file pour les flux multiplexés
- Migration de connexion transparente (passage Wi-Fi → 4G sans interruption)
- 0-RTT pour les reconnexions vers des serveurs connus

TCP reste compétitif sur des réseaux stables et peu chargés où le HOL blocking est marginal.

**4. Quel protocole semble le plus sécurisé ?**

**QUIC** est intrinsèquement plus sécurisé car :
- Le chiffrement TLS 1.3 est **obligatoire** (impossible de déployer QUIC non chiffré)
- Les en-têtes de transport sont partiellement protégés (numéros de paquet chiffrés)
- La protection contre la modification des paquets est intégrée
- Moins de métadonnées exposées aux observateurs passifs

TCP sans TLS est non chiffré ; même avec TLS, les métadonnées TCP restent exposées.

**5. Dans quels contextes TCP reste-t-il pertinent ?**

TCP reste pertinent dans les contextes suivants :
- **Protocoles ne nécessitant pas HTTP** : SSH, SMTP, bases de données, FTP
- **Environnements bloquant UDP** : certains firewalls d'entreprise bloquent UDP/443
- **Systèmes embarqués** avec ressources limitées ne supportant pas QUIC
- **Protocoles legacy** qui ne seront pas migrés vers HTTP/3
- **Analyse et débogage réseau** : TCP est plus transparent et plus facile à instrumenter
- **Réseaux très stables** (LAN, datacenters) où les avantages de QUIC sont marginaux

**6. Dans quels contextes QUIC semble-t-il préférable ?**

QUIC est préférable dans les contextes suivants :
- **Navigation web moderne** : HTTP/3 améliore le chargement des pages avec de nombreuses ressources
- **Réseaux mobiles** : les changements de réseau (Wi-Fi → 4G) sont transparents grâce aux Connection ID
- **Réseaux à forte latence ou pertes** : le HOL blocking de TCP pénalise davantage dans ces conditions
- **Streaming vidéo** : la reprise rapide après perte améliore l'expérience utilisateur
- **Applications temps réel** : jeux en ligne, visioconférence bénéficiant de la latence réduite
- **CDN et services cloud** : Cloudflare, Google, Meta utilisent massivement QUIC/HTTP/3

---

## Annexe — Filtres Wireshark utilisés

```wireshark
# Filtrer tout le trafic TCP
tcp

# Filtrer tout le trafic UDP
udp

# Filtrer les paquets QUIC
quic

# Filtrer TCP sur le port 443 (HTTPS)
tcp.port == 443

# Filtrer UDP sur le port 443 (QUIC/HTTP3)
udp.port == 443

# Filtrer les échanges TLS
tls

# Filtrer le trafic HTTP non chiffré
http

# Filtrer le handshake TCP (SYN uniquement)
tcp.flags.syn == 1 && tcp.flags.ack == 0

# Filtrer le SYN-ACK
tcp.flags.syn == 1 && tcp.flags.ack == 1

# Identifier les retransmissions TCP
tcp.analysis.retransmission

# Filtrer les paquets QUIC Initial (handshake)
quic.long.packet_type == 0

# Filtrer par adresse IP serveur
ip.addr == 93.184.216.34
```

---

## Conclusion

Ce TP a permis de mettre en évidence les différences fondamentales entre TCP et QUIC à travers l'analyse de captures Wireshark réelles.

**TCP** est un protocole mature, universellement supporté, transparent à l'analyse réseau, mais présentant des limitations structurelles : latence d'établissement élevée, blocage de tête de file, chiffrement non intégré.

**QUIC** représente une évolution majeure : en s'appuyant sur UDP et en intégrant TLS 1.3 nativement, il réduit la latence d'établissement, élimine le HOL blocking, protège mieux les métadonnées, et supporte la migration de connexion. Ces avantages en font le protocole de choix pour les applications web modernes et les environnements mobiles.

L'analyse Wireshark illustre également un paradoxe : plus un protocole est sécurisé, moins il est analysable. QUIC, en chiffrant davantage, complique le travail des administrateurs réseau et des outils d'inspection, tout en offrant une meilleure protection à l'utilisateur final.
