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
