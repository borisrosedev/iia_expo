# TCP Reno — Ressource théorique

> Document de référence pour l'exposé. Public : étudiants en 3ᵉ année de licence informatique connaissant TCP/IP.

---

## 1. Pourquoi un contrôle de congestion ?

### 1.1 Le contexte historique

En octobre 1986, le réseau NSFNET — ancêtre d'Internet — connaît son premier **effondrement par congestion** (*congestion collapse*). Sur la liaison entre le LBL et l'UC Berkeley (400 mètres physiques), le débit utile s'effondre de **32 kbit/s à 40 bit/s**, soit un facteur ~1000.

Le coupable : trop d'émetteurs TCP qui retransmettent des paquets perdus, saturant un peu plus les routeurs déjà engorgés. Plus le réseau souffre, plus on lui envoie de trafic — c'est un effet d'avalanche.

**Van Jacobson** publie en 1988 *Congestion Avoidance and Control*, qui pose les bases du contrôle de congestion moderne dans TCP. La première implémentation est **TCP Tahoe** (4.3BSD-Tahoe, 1988). Elle est raffinée deux ans plus tard dans **TCP Reno** (4.3BSD-Reno, 1990), du nom de la ville du Nevada où s'est tenue la conférence Usenix correspondante.

### 1.2 Le problème à résoudre

TCP doit répondre à deux questions opposées :

- **À quelle vitesse émettre ?** Trop lent = gaspillage de bande passante. Trop vite = congestion.
- **Comment réagir aux pertes ?** Une perte est l'indice principal d'une congestion (sur réseau filaire).

La difficulté : l'émetteur ne connaît **ni la capacité du chemin**, ni le nombre de flux concurrents. Il doit la **deviner dynamiquement** à partir des seuls signaux qu'il reçoit (ACK, timeouts).

---

## 2. Rappels indispensables

### 2.1 Les fenêtres

TCP utilise un mécanisme de **fenêtre glissante**. À tout instant, l'émetteur peut avoir au plus *W* octets « en vol » (envoyés mais non acquittés).

| Fenêtre | Origine | Rôle |
|---|---|---|
| `rwnd` (receive window) | annoncée par le récepteur | éviter que le récepteur ne soit débordé (contrôle de flux) |
| `cwnd` (congestion window) | calculée par l'émetteur | éviter de surcharger le réseau (contrôle de congestion) |

> **Fenêtre effective = min(cwnd, rwnd)**

`cwnd` est la variable centrale de TCP Reno. C'est elle qu'on fait varier dynamiquement.

### 2.2 Les autres variables clés

- **MSS** (*Maximum Segment Size*) : taille maximale d'un segment TCP. `cwnd` est exprimée en multiples de MSS.
- **ssthresh** (*slow start threshold*) : seuil qui sépare les deux régimes de croissance de `cwnd`.
- **RTT** (*Round Trip Time*) : temps aller-retour mesuré.
- **RTO** (*Retransmission Timeout*) : délai avant qu'on considère un segment comme perdu.

### 2.3 Les deux signaux de congestion

Reno distingue deux types de pertes, qui n'ont pas la même gravité :

1. **Timeout (RTO expire)** — signal **fort**. Probablement plus aucun ACK ne revient : le chemin est très congestionné, voire coupé.
2. **3 ACK dupliqués** — signal **faible**. Le récepteur continue de renvoyer des ACK, donc la connexion vit ; un seul segment manque à l'appel.

C'est cette distinction — absente de Tahoe — qui fait toute la différence de Reno.

---

## 3. Les quatre mécanismes de TCP Reno

Reno combine quatre algorithmes, formalisés ensuite par la **RFC 5681**.

### 3.1 Slow Start (démarrage lent)

Au début de la connexion, l'émetteur ne sait rien du chemin. Il ne peut pas se permettre d'envoyer une grosse rafale qui ferait s'effondrer un lien lent.

- **Initialisation** : `cwnd = 1 MSS` (historiquement ; aujourd'hui RFC 6928 autorise jusqu'à 10 MSS).
- **Croissance** : à chaque ACK reçu, `cwnd += 1 MSS`.
- **Conséquence** : `cwnd` **double à chaque RTT** — c'est une croissance **exponentielle**.

> Le nom « slow start » est trompeur : c'est lent au début mais ça accélère très vite. « Slow » s'oppose à l'ancien comportement qui consistait à envoyer immédiatement toute la fenêtre annoncée.

**Exemple numérique** : `cwnd` part à 1, ssthresh = 16 MSS.

| RTT | cwnd (MSS) |
|---|---|
| 0 | 1 |
| 1 | 2 |
| 2 | 4 |
| 3 | 8 |
| 4 | 16 → on bascule en Congestion Avoidance |

On reste en slow start tant que `cwnd < ssthresh`.

### 3.2 Congestion Avoidance (évitement de congestion)

Une fois `cwnd ≥ ssthresh`, on suppose qu'on s'approche de la capacité du lien. Plus question de doubler — il faut sonder prudemment.

- **Croissance** : `cwnd += 1 MSS` **par RTT** (et non par ACK). En pratique : `cwnd += MSS × MSS / cwnd` à chaque ACK.
- C'est une croissance **linéaire** (additive).

C'est la phase **AI** d'**AIMD** (*Additive Increase, Multiplicative Decrease*) : on monte tout doucement jusqu'à ce que le réseau crache une perte, qui sert de signal.

### 3.3 Fast Retransmit (retransmission rapide)

Quand un récepteur reçoit un segment hors séquence (parce qu'un précédent manque), il **réacquitte le dernier segment reçu dans l'ordre**. C'est un **ACK dupliqué**.

Tahoe attendait que le timer RTO expire pour retransmettre — ce qui peut prendre plusieurs centaines de ms. Reno fait mieux :

> **Règle Reno** : à la réception du **3ᵉ ACK dupliqué** consécutif, l'émetteur retransmet **immédiatement** le segment supposé perdu, sans attendre l'expiration du RTO.

Pourquoi 3 et pas 1 ou 2 ? Parce qu'un ou deux ACK dupliqués peuvent venir d'un simple **réordonnancement** des paquets dans le réseau, pas d'une perte. Trois est le compromis empirique entre réactivité et faux positifs.

### 3.4 Fast Recovery (récupération rapide) — **LA nouveauté de Reno**

C'est le point qui distingue vraiment Reno de Tahoe.

**Tahoe** : sur 3 ACK dupliqués, il fait `cwnd = 1`, `ssthresh = cwnd/2`, et **repart en slow start**. Le pipe se vide entièrement, on perd plusieurs RTT à le remplir à nouveau.

**Reno** raisonne autrement : si on reçoit des ACK dupliqués, c'est que **des paquets continuent d'arriver** au récepteur. La connexion respire encore. Donc inutile de tout casser.

**Procédure Fast Recovery** :

1. À la réception du 3ᵉ ACK dupliqué :
   - `ssthresh = cwnd / 2`
   - `cwnd = ssthresh + 3 × MSS` (les 3 segments « en vol » correspondant aux 3 dup ACK)
   - Retransmettre le segment manquant
2. Pour chaque ACK dupliqué supplémentaire reçu : `cwnd += 1 MSS` (on gonfle artificiellement la fenêtre, car ces dup ACK signalent que des segments quittent le réseau)
3. À la réception du **nouvel ACK** (qui acquitte enfin le segment retransmis) :
   - `cwnd = ssthresh`
   - Reprise en **Congestion Avoidance** (et non en slow start !)

Résultat : `cwnd` est seulement **divisée par deux** au lieu de tomber à 1. C'est la phase **MD** (*Multiplicative Decrease*) d'AIMD.

En cas de **timeout**, en revanche, Reno reste prudent comme Tahoe : `ssthresh = cwnd/2`, `cwnd = 1`, retour en slow start.

---

## 4. La courbe en dents de scie

Le comportement combiné donne la fameuse **courbe en dents de scie** de `cwnd` au cours du temps :

```
cwnd ▲
     │              ╱│           ╱│         ╱│
     │            ╱  │         ╱  │       ╱  │
     │          ╱    │       ╱    │     ╱    │
     │        ╱      ▼     ╱      ▼   ╱      ▼  ← perte (3 dup ACK)
     │      ╱        │   ╱        │ ╱        │     → cwnd /= 2
     │    ╱          │ ╱          │ ╱        │
     │  ╱ slow start │             │
     │╱              │             │
     └────────────────────────────────────────▶ temps
```

- Montée raide initiale = slow start (exponentielle)
- Pentes douces = congestion avoidance (linéaire)
- Chutes verticales = perte détectée → division par 2

C'est cette signature visuelle qu'on associe à Reno.

---

## 5. Tahoe vs Reno : la comparaison directe

| Événement | Tahoe | Reno |
|---|---|---|
| Timeout | `cwnd = 1`, slow start | `cwnd = 1`, slow start |
| 3 ACK dupliqués | `cwnd = 1`, slow start | Fast Recovery : `cwnd /= 2`, congestion avoidance |
| Comportement de `cwnd` | dents de scie + chutes brutales à zéro | dents de scie classiques |
| Efficacité | moins bonne sur pertes isolées | meilleure sur pertes isolées |

---

## 6. Les limites de Reno

Reno a un défaut connu : il gère mal **plusieurs pertes dans une même fenêtre**.

Le problème : Fast Recovery ne récupère **qu'une seule perte par RTT**. Si deux segments sont perdus dans la même fenêtre, l'émetteur sort de Fast Recovery avec le premier nouvel ACK reçu, mais le second segment perdu déclenche alors… un nouveau cycle de Fast Recovery, voire un timeout. Dans le pire cas, on tombe en RTO, `cwnd = 1`, et tous les bénéfices sont perdus.

C'est ce qu'on appelle parfois le **problème des pertes corrélées**.

### Successeurs

| Algorithme | Année | Apport principal |
|---|---|---|
| **NewReno** (RFC 6582) | 1999 | Reste en Fast Recovery jusqu'à acquittement de **tous** les segments en vol au moment de la perte |
| **TCP SACK** (RFC 2018) | 1996 | Le récepteur indique précisément **quels** segments lui manquent (acquittements sélectifs) |
| **TCP Vegas** | 1994 | Détecte la congestion via la **variation du RTT**, pas seulement les pertes |
| **CUBIC** | 2006 | Fonction cubique de la croissance, optimisée pour réseaux haut débit longue distance — **défaut Linux depuis 2006** |
| **BBR** (Google) | 2016 | Modélise explicitement la bande passante et le RTT, n'utilise pas les pertes comme signal |

---

## 7. Pourquoi étudie-t-on encore Reno ?

1. **Référence pédagogique** : c'est l'archétype d'AIMD, le premier à combiner les 4 mécanismes qui structurent encore tout TCP moderne.
2. **Reno-friendliness** : tout nouvel algorithme de contrôle de congestion doit prouver qu'il **partage équitablement** la bande passante avec un flux Reno. C'est un critère de validation des RFC.
3. **Simplicité** : Reno tient en quelques lignes de pseudo-code, ce qui en fait un excellent terrain d'analyse mathématique (formule de Mathis, modèle de débit ~ MSS / (RTT × √p)).

---

## 8. Pour aller plus loin

- **RFC 5681** — *TCP Congestion Control* (la version officielle moderne, intègre Reno)
- **RFC 2001** — *TCP Slow Start, Congestion Avoidance, Fast Retransmit, and Fast Recovery Algorithms* (Reno historique)
- **RFC 6582** — *The NewReno Modification to TCP's Fast Recovery Algorithm*
- Van Jacobson, *Congestion Avoidance and Control*, SIGCOMM 1988
- Kurose & Ross, *Computer Networking: A Top-Down Approach*, chapitre 3
