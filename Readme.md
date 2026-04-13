# Paxos — Consensus distribué dans les réseaux

## 1. Pourquoi le consensus ? Le problème de fond

Dans le cours, nous avons vu que TCP garantit la fiabilité entre **deux hôtes**. Mais que se passe-t-il quand **plusieurs serveurs** doivent se mettre d'accord sur une même valeur ou un même état ?

- TCP résout le problème de communication point-à-point (fiabilité, ordre, intégrité).
- MPTCP permet de gérer plusieurs chemins pour une même session.
- Mais aucun de ces protocoles ne répond à la question : **comment plusieurs nœuds d'un système distribué se mettent-ils d'accord malgré des pannes ?**

C'est le **problème du consensus distribué**.

> **Définition** : Le consensus distribué est le mécanisme par lequel un ensemble de nœuds réseau parvient à s'accorder sur une valeur commune, même en présence de pannes ou de délais imprévisibles.

---

## 2. Contexte historique

- **1989** : Leslie Lamport publie *The Part-Time Parliament* (refusé par des reviewers, jugé trop fantaisiste).
- **1998** : Publication finalement acceptée dans *ACM Transactions on Computer Systems*.
- **2001** : Lamport publie *Paxos Made Simple*, une version plus accessible.
- **2013** : Lamport reçoit le Prix Turing pour ses travaux sur les systèmes distribués.

> Paxos est aujourd'hui le fondement de nombreux systèmes : **Google Chubby**, **Apache Zookeeper**, **etcd** (Kubernetes), **CockroachDB**.

---

## 3. Le problème que Paxos résout

### 3.1 Les hypothèses du réseau

Les **middleboxes**, les **pertes de paquets** (réseaux lossy), les **latences variables** (high-latency connections) sont des réalités des réseaux modernes.

Dans un système distribué, on fait face à :
- Des **nœuds qui tombent en panne** (crash failure)
- Des **messages perdus ou retardés** (comme les segments TCP perdus → retransmission)
- Des **nœuds qui redémarrent** avec une mémoire partielle

### 3.2 Propriétés requises du consensus

| Propriété | Description |
|-----------|-------------|
| **Validité** | La valeur décidée a été proposée par un nœud réel |
| **Accord** | Tous les nœuds corrects décident la même valeur |
| **Terminaison** | Tout nœud correct finit par décider |
| **Tolérance aux pannes** | Le système fonctionne si la majorité des nœuds est disponible |

---

## 4. Les acteurs de Paxos

Paxos définit trois rôles (un même nœud peut jouer plusieurs rôles) :

| Rôle | Description | Analogie réseau |
|------|-------------|-----------------|
| **Proposer** | Propose une valeur | Client TCP qui initie une connexion |
| **Acceptor** | Vote pour ou contre la proposition | Serveur qui répond au SYN |
| **Learner** | Apprend la valeur décidée | Récepteur final des données |

> La **majorité** (quorum) est la clé : si N acceptors, on a besoin de **(N/2)+1** votes pour valider.

---

## 5. Le protocole en deux phases

### Phase 1 — Prepare / Promise (analogie : SYN / SYN-ACK)

```
Proposer                    Acceptors
    |                           |
    |--- Prepare(n) ----------->|   n = numéro de round
    |                           |
    |<-- Promise(n, v_ancien) --|   "Je ne réponds plus aux anciens rounds"
```

- Le Proposer choisit un numéro de round `n` unique et croissant.
- Il envoie `Prepare(n)` à la majorité des Acceptors.
- Chaque Acceptor répond `Promise(n)` : il s'engage à **ne plus accepter** de proposition avec un numéro inférieur à `n`.

> **Lien avec TCP Fast Open** : comme TFO utilise un cookie pour les connexions ultérieures, Paxos utilise le numéro de round pour établir une "priorité" entre plusieurs proposers concurrents.

### Phase 2 — Accept / Accepted (analogie : ACK + DATA)

```
Proposer                    Acceptors                  Learners
    |                           |                          |
    |--- Accept(n, v) --------->|                          |
    |                           |                          |
    |<-- Accepted(n, v) --------|                          |
    |                           |--- Notify(v) ----------->|
```

- Le Proposer envoie `Accept(n, v)` avec la valeur à proposer.
- Si les Acceptors n'ont pas promis à un numéro plus grand, ils acceptent et notifient les Learners.
- Une fois la majorité obtenue, la valeur `v` est **décidée** (committed).

---

## 6. Tolérance aux pannes : la règle du quorum

Paxos tolère **f pannes** avec **2f+1 nœuds**.

| Nœuds totaux | Pannes tolérées | Quorum requis |
|:------------:|:---------------:|:-------------:|
| 3 | 1 | 2 |
| 5 | 2 | 3 |
| 7 | 3 | 4 |

> **Lien avec MPTCP** : tout comme MPTCP maintient la session si l'une des interfaces tombe (failover), Paxos maintient le consensus si une minorité de nœuds est défaillante.

---

## 7. Le problème de liveness : les proposers concurrents

Un cas particulier : **deux proposers** s'affrontent indéfiniment.

```
Proposer A : Prepare(n=1) → Promise → Prepare(n=3) par B → plus de quorum
Proposer B : Prepare(n=2) → Promise → Prepare(n=5) par A → plus de quorum
...
```

C'est le problème de **liveness** : le système est sûr (aucune mauvaise valeur) mais ne progresse pas.

**Solution : Multi-Paxos avec un leader élu**  
On désigne un **leader unique** (proposer distingué) qui concentre toutes les propositions. Les autres proposers renoncent.

> **Lien avec les algorithmes de contrôle de congestion** : comme AIMD (Additive Increase, Multiplicative Decrease) en TCP, Paxos gère la contention entre participants, mais à un niveau logique plutôt que réseau.

---

## 8. Multi-Paxos : optimisation pour les systèmes réels

Dans la pratique, on veut décider **une séquence de valeurs** (log de commandes), pas une seule.

- **Multi-Paxos** : le leader est stable → on saute la Phase 1 pour les rounds suivants.
- **Instance Paxos** : chaque entrée du log est une instance indépendante.

```
Instance 1 : valeur "Set x=5" → décidée
Instance 2 : valeur "Set y=3" → décidée
Instance 3 : valeur "Delete z" → en cours...
```

> C'est exactement ce que font **etcd** (Kubernetes), **Google Spanner** et **CockroachDB**.

---

## 9. Variantes modernes de Paxos

| Variante | Particularité | Utilisé par |
|----------|---------------|-------------|
| **Raft** | Plus simple à comprendre, même garanties | etcd, CockroachDB, TiKV |
| **Viewstamped Replication** | Précède Paxos, même famille | Systèmes académiques |
| **Zab (ZooKeeper)** | Optimisé pour les broadcasts totalement ordonnés | Apache Zookeeper |
| **EPaxos** | Pas de leader unique, meilleure latence | Recherche / expérimental |

> **Raft** est souvent préféré à Paxos en industrie pour sa clarté — comme MPTCP est préféré à SCTP pour la compatibilité avec l'existant.

---

## 10. Paxos et les protocoles : tableau de correspondance

| Concept | Équivalent Paxos |
|----------------|-----------------|
| TCP 3-way handshake | Phase 1 Prepare/Promise |
| Numéro de séquence TCP | Numéro de round `n` |
| ACK / retransmission | Accepted / retry |
| Fenêtre glissante (cwnd) | Quorum (majorité) |
| MPTCP multi-chemins | Paxos multi-acceptors |
| Slow Start / Fast Retransmit | Backoff entre proposers concurrents |
| Middleboxes | Nœuds byzantins (extension) |

---

## 11. Cas d'usage réels

### Google Chubby (2006)
- Service de verrou distribué.
- Utilise Paxos pour maintenir la cohérence d'un petit groupe de répliques.
- Utilisé en interne par Bigtable, GFS, MapReduce.

### etcd (Kubernetes)
- Base de données clé-valeur distribuée.
- Utilise **Raft** (inspiré de Paxos) pour stocker la configuration du cluster.
- Si etcd tombe, Kubernetes ne peut plus prendre de décision.

### Apache ZooKeeper
- Coordination de services distribués (Kafka, HBase, Hadoop).
- Utilise **Zab**, variante de Multi-Paxos.

---

## 12. Limites et critiques

- **Complexité** : Paxos est difficile à implémenter correctement (comme SCTP vs TCP/UDP).
- **Latence** : 2 allers-retours minimum avant de décider — lien avec la latence dans le cours.
- **Pas de protection contre les nœuds malveillants** : Paxos assume des *crash failures*, pas des *Byzantine failures* (nœuds qui mentent).
- **Liveness non garantie** sans leader élu.

> Pour les pannes byzantines : **PBFT** (Practical Byzantine Fault Tolerance) ou **BFT** sont utilisés — mais avec des coûts bien plus élevés.

---

## 13. Résumé

```
Problème         → Comment décider à plusieurs malgré les pannes ?
Solution Paxos   → Protocole en 2 phases avec quorum de majorité
Acteurs          → Proposer / Acceptor / Learner
Tolérance        → f pannes avec 2f+1 nœuds
En pratique      → Multi-Paxos avec leader (Raft, Zab, Chubby, etcd)
Lien RSX102      → TCP fiable à 2 hôtes → Paxos fiable à N hôtes
```

---

## Références

- Lamport, L. (1998). *The Part-Time Parliament*. ACM TOCS.
- Lamport, L. (2001). *Paxos Made Simple*. ACM SIGACT News.
- Ongaro, D. & Ousterhout, J. (2014). *In Search of an Understandable Consensus Algorithm (Raft)*.
- Google (2006). *The Chubby Lock Service for Loosely-Coupled Distributed Systems*.
- Cours — Boris Rose (2026).

---

## 14. Glossaire

| Terme | Définition |
|-------|------------|
| **ACK (Acknowledgement)** | Message de confirmation envoyé par un récepteur pour signaler qu'il a bien reçu des données. En TCP, chaque segment reçu donne lieu à un ACK. |
| **AIMD (Additive Increase, Multiplicative Decrease)** | Algorithme de contrôle de congestion TCP : la fenêtre d'émission augmente linéairement en l'absence de perte, et est divisée par deux dès qu'une perte est détectée. |
| **Bigtable** | Système de stockage distribué développé par Google, utilisé pour structurer de très grands volumes de données. Utilise Chubby pour la coordination. |
| **Byzantine failure (panne byzantine)** | Type de panne où un nœud ne se contente pas de tomber en panne mais envoie des informations incorrectes ou contradictoires. Plus difficile à tolérer que les crash failures. |
| **BFT (Byzantine Fault Tolerance)** | Famille de protocoles permettant à un système distribué de continuer à fonctionner correctement même en présence de nœuds byzantins. |
| **Crash failure (panne franche)** | Type de panne où un nœud s'arrête brutalement et cesse toute communication. C'est le modèle de panne assumé par Paxos. |
| **CockroachDB** | Base de données relationnelle distribuée et tolérante aux pannes, utilisant Raft pour assurer la cohérence des données entre répliques. |
| **EPaxos (Egalitarian Paxos)** | Variante de Paxos sans leader unique : toutes les répliques peuvent proposer des valeurs, ce qui réduit la latence en exploitant la localité géographique. |
| **etcd** | Base de données clé-valeur distribuée utilisée comme magasin de configuration central de Kubernetes. Repose sur l'algorithme Raft. |
| **Fast Retransmit** | Mécanisme TCP permettant de retransmettre un segment perdu sans attendre l'expiration du timer de retransmission, dès réception de trois ACK dupliqués. |
| **Fenêtre glissante (cwnd, congestion window)** | Mécanisme TCP limitant le nombre de segments pouvant être envoyés sans accusé de réception, afin de contrôler le débit et éviter la congestion. |
| **GFS (Google File System)** | Système de fichiers distribué développé par Google, conçu pour stocker de très grands fichiers sur des grappes de serveurs ordinaires. |
| **Google Chubby** | Service de verrou distribué développé par Google, basé sur Paxos. Il sert à coordonner l'accès à des ressources partagées entre des services distribués. |
| **Google Spanner** | Base de données relationnelle globalement distribuée de Google, offrant des transactions cohérentes à l'échelle mondiale grâce à Multi-Paxos et des horloges atomiques. |
| **HBase** | Base de données NoSQL distribuée, modélisée d'après Bigtable, fonctionnant sur Hadoop et utilisant ZooKeeper pour la coordination. |
| **Hadoop** | Framework open-source de traitement distribué de grands volumes de données. Utilise ZooKeeper pour la coordination de ses composants. |
| **High-latency connection** | Connexion réseau présentant un temps de propagation élevé, typiquement les liaisons satellite ou intercontinentales. |
| **Instance Paxos** | Exécution indépendante du protocole Paxos pour décider d'une seule valeur dans un log. Multi-Paxos enchaîne des instances numérotées pour construire un log ordonné. |
| **Kafka** | Plateforme de streaming d'événements distribuée, utilisant ZooKeeper (ou KRaft depuis les versions récentes) pour la coordination de ses brokers. |
| **Kubernetes** | Système d'orchestration de conteneurs open-source. Sa configuration est stockée dans etcd, qui repose sur Raft. |
| **Leader (dans Multi-Paxos)** | Nœud désigné comme seul Proposer actif dans Multi-Paxos. Sa stabilité permet de supprimer la Phase 1 pour les rounds successifs et d'éviter les conflits entre proposers. |
| **Liveness** | Propriété d'un protocole garantissant qu'il progresse effectivement vers une décision. Un protocole sûr mais sans liveness peut bloquer indéfiniment. |
| **Log de commandes** | Séquence ordonnée d'opérations à appliquer à un état partagé. Multi-Paxos est typiquement utilisé pour construire un tel log de manière cohérente entre répliques. |
| **Lossy network (réseau à pertes)** | Réseau dans lequel des paquets peuvent être perdus en transit, notamment à cause de la congestion ou de liens défaillants. |
| **MapReduce** | Modèle de programmation distribué de Google pour traiter de grands volumes de données en parallèle sur un cluster. |
| **Middlebox** | Équipement réseau intermédiaire (pare-feu, NAT, proxy, load balancer) qui peut inspecter ou modifier le trafic entre deux hôtes. |
| **MPTCP (Multipath TCP)** | Extension de TCP permettant d'utiliser simultanément plusieurs chemins réseau (interfaces) pour une même session, améliorant le débit et la résilience. |
| **Nœud (node)** | Machine participante d'un système distribué. Un nœud peut jouer un ou plusieurs rôles dans Paxos (Proposer, Acceptor, Learner). |
| **PBFT (Practical Byzantine Fault Tolerance)** | Protocole de consensus tolérant les pannes byzantines, permettant à un système de fonctionner correctement même si certains nœuds se comportent de manière malveillante. Coût en communication plus élevé que Paxos. |
| **Quorum** | Sous-ensemble minimum de nœuds devant participer à une décision pour qu'elle soit valide. Dans Paxos, le quorum est la majorité stricte : ⌊N/2⌋ + 1 nœuds. |
| **Raft** | Algorithme de consensus conçu pour être plus compréhensible que Paxos, avec les mêmes garanties. Il structure explicitement l'élection de leader et la réplication de log. |
| **Réplique** | Copie d'un état ou d'une donnée maintenue sur plusieurs nœuds pour assurer la disponibilité et la tolérance aux pannes. |
| **Round (numéro de round)** | Identifiant numérique croissant associé à chaque tentative de proposition dans Paxos. Permet de totalement ordonner les propositions et de résoudre les conflits. |
| **SCTP (Stream Control Transmission Protocol)** | Protocole de transport alternatif à TCP et UDP, offrant le multi-homing et le multi-streaming, mais peu déployé en pratique en raison de la compatibilité limitée avec les middleboxes. |
| **Slow Start** | Phase initiale du contrôle de congestion TCP où la fenêtre d'émission croît exponentiellement jusqu'à un seuil, permettant de sonder rapidement la capacité du réseau. |
| **SYN / SYN-ACK** | Premiers messages de l'établissement de connexion TCP en trois étapes (three-way handshake) : le client envoie SYN, le serveur répond SYN-ACK, puis le client confirme avec ACK. |
| **TCP Fast Open (TFO)** | Extension de TCP permettant d'envoyer des données dès le premier paquet SYN grâce à un cookie préalablement échangé, réduisant la latence d'établissement de connexion. |
| **TiKV** | Base de données clé-valeur distribuée transactionnelle, utilisée notamment par TiDB. Repose sur Raft pour la réplication. |
| **Viewstamped Replication** | Protocole de consensus distribué publié par Liskov et Cowling, antérieur à Paxos et proposant des garanties équivalentes. |
| **Zab (ZooKeeper Atomic Broadcast)** | Protocole de diffusion atomique totalement ordonné utilisé par Apache ZooKeeper, inspiré de Multi-Paxos et optimisé pour la coordination de services distribués. |
