# Raft — Algorithme de consensus distribué
### Tout comprendre sur Raft, de zéro à la production

---

## Table des matières

1. [Le problème des systèmes distribués](#1-le-problème-des-systèmes-distribués)
2. [Qu'est-ce que Raft ?](#2-quest-ce-que-raft-)
3. [Les trois rôles d'un serveur](#3-les-trois-rôles-dun-serveur)
4. [L'élection du Leader](#4-lélection-du-leader)
5. [La réplication du log](#5-la-réplication-du-log)
6. [La sécurité — ce que Raft garantit](#6-la-sécurité--ce-que-raft-garantit)
7. [Tolérance aux pannes](#7-tolérance-aux-pannes)
8. [Raft vs Paxos](#8-raft-vs-paxos)
9. [Raft dans le monde réel](#9-raft-dans-le-monde-réel)
10. [Limites de Raft](#10-limites-de-raft)
11. [Glossaire](#11-glossaire)

---

## 1. Le problème des systèmes distribués

### Pourquoi plusieurs serveurs ?

Une application critique — une banque, un service de paiement, une base de données cloud — ne peut pas tourner sur un seul serveur. Si ce serveur tombe en panne, tout s'arrête. Pour garantir la disponibilité, on répartit l'application sur plusieurs serveurs qui travaillent ensemble : c'est ce qu'on appelle un **système distribué**.

### Le problème de cohérence

Dès qu'on a plusieurs serveurs, un problème fondamental apparaît : comment s'assurer qu'ils ont tous les mêmes données ? Sur un réseau, les messages peuvent se perdre, arriver dans le mauvais ordre, ou une machine peut s'éteindre à n'importe quel moment sans prévenir.

Imaginez trois serveurs qui stockent la valeur d'une variable `x`. Un client envoie la commande "mettre x à 5". Cette commande arrive au serveur A et au serveur C, mais le message se perd avant d'atteindre le serveur B. Maintenant A et C ont `x = 5`, mais B a toujours l'ancienne valeur. Les trois serveurs sont en désaccord.

Si un autre client lit `x` depuis le serveur B, il obtient une valeur incorrecte. C'est une **incohérence** — un problème grave pour toute application sérieuse.

### Le problème du consensus

Le **consensus** est le problème fondamental qui consiste à faire en sorte que plusieurs machines s'accordent sur une même valeur, même en présence de pannes. Formellement, un algorithme de consensus doit garantir trois propriétés :

- **Accord** : toutes les machines correctes décident de la même valeur.
- **Validité** : la valeur décidée a été proposée par une des machines.
- **Terminaison** : toutes les machines correctes finissent par prendre une décision.

### Le théorème FLP

En 1985, les chercheurs Fischer, Lynch et Patterson ont prouvé qu'il est **impossible** de résoudre le consensus de manière déterministe dans un système asynchrone où même un seul processus peut tomber en panne. C'est le célèbre **théorème FLP**.

La solution pratique : accepter un modèle partiellement synchrone — le réseau peut être lent, mais pas indéfiniment. C'est l'hypothèse que font Raft et tous les algorithmes de consensus pratiques.

---

## 2. Qu'est-ce que Raft ?

### Définition

Raft est un **algorithme de consensus** conçu pour être facile à comprendre et à implémenter. Il permet à un groupe de serveurs de maintenir un **log répliqué** — un journal d'opérations identique sur tous les serveurs — même en présence de pannes.

L'acronyme RAFT signifie **Reliable, Replicated, Redundant And Fault-Tolerant**.

### Origines

Raft a été créé par **Diego Ongaro** et **John Ousterhout** à l'Université de Stanford. Le papier original, intitulé *"In Search of an Understandable Consensus Algorithm"*, a été publié en 2014 et a reçu le **Best Paper Award** à la conférence USENIX Annual Technical Conference.

La motivation principale était de créer un algorithme aussi correct et performant que Paxos — le standard de l'époque — mais beaucoup plus facile à comprendre et à implémenter correctement.

### Le modèle de machine à états répliquée

Raft s'appuie sur le modèle de **machine à états répliquée** (*Replicated State Machine*). L'idée est la suivante :

1. Chaque serveur maintient un **log** — une liste ordonnée de commandes.
2. Si deux serveurs ont le même log, ils ont forcément le même état interne.
3. Le rôle de Raft est de s'assurer que tous les logs sont identiques.

Ainsi, même si certains serveurs tombent en panne, tant qu'une majorité reste active, le log reste cohérent et le service continue de fonctionner.

---

## 3. Les trois rôles d'un serveur

Dans un cluster Raft, chaque serveur est à tout moment dans l'un de ces trois états :

### Leader

Il y a **exactement un Leader** à la fois dans le cluster (ou zéro, pendant une élection). Le Leader est responsable de :

- Recevoir toutes les requêtes des clients.
- Ajouter les nouvelles entrées dans son log et les répliquer sur les Followers.
- Envoyer régulièrement des **heartbeats** (messages "je suis vivant") à tous les Followers.
- Décider quand une entrée est suffisamment répliquée pour être considérée comme validée.

Si un client contacte un Follower par erreur, le Follower le redirige vers le Leader.

### Follower

C'est l'état par défaut. Un Follower est **passif** : il ne prend aucune initiative, il répond simplement aux requêtes du Leader et des Candidats. Il maintient un minuteur — le **timeout d'élection**. Si ce minuteur expire sans avoir entendu le Leader, le Follower devient Candidat.

### Candidate

État **transitoire** pendant une élection. Un Follower dont le timeout expire se déclare Candidat, demande des votes aux autres serveurs, et cherche à devenir Leader. Si l'élection échoue (égalité, timeout), il recommence un nouveau cycle.

### Transitions entre états

```
Follower  ──(timeout)──►  Candidate  ──(majorité de votes)──►  Leader
   ▲                           │                                   │
   │◄──────(découvre un        │◄──────(découvre un Leader         │
   │        Leader plus récent)│        ou terme plus récent)──────┘
   └───────────────────────────┘
```

---

## 4. L'élection du Leader

### Les termes

Raft divise le temps en **termes** (*terms*) — des périodes numérotées qui commencent chacune par une élection. Le numéro de terme est un compteur monotone : il augmente à chaque nouvelle élection et ne diminue jamais.

Chaque serveur connaît le terme courant. Si un serveur reçoit un message avec un terme plus élevé que le sien, il met à jour son terme et repasse en état Follower. Si un serveur reçoit un message avec un terme plus bas, il le rejette.

Le terme sert à détecter les informations obsolètes et à éviter qu'un ancien Leader "fantôme" — isolé temporairement du réseau — reprenne son rôle après être revenu.

### Déclenchement d'une élection

Chaque Follower maintient un minuteur dont la durée est choisie **aléatoirement** dans un intervalle, typiquement entre 150 et 300 millisecondes. Si le minuteur expire sans recevoir de heartbeat du Leader, le Follower :

1. Incrémente son numéro de terme.
2. Passe en état Candidat.
3. Vote pour lui-même.
4. Envoie un message `RequestVote` à tous les autres serveurs.

### Le vote

Chaque serveur vote pour **au plus un candidat par terme**, selon le principe du premier arrivé, premier servi. Cependant, un serveur refuse de voter pour un Candidat si son propre log est plus à jour que celui du Candidat. Cette règle garantit que le Leader élu a toujours le log le plus complet.

### Résolution de l'élection

- Si un Candidat reçoit les votes de la **majorité** des serveurs, il devient Leader et envoie immédiatement des heartbeats pour signaler son élection.
- Si deux Candidats obtiennent le même nombre de votes (égalité), personne ne gagne. Chaque Candidat attend un nouveau timeout aléatoire, puis recommence. L'aléatoire dans les timeouts garantit que les égalités sont rares et se résolvent rapidement.
- Si un Candidat reçoit un heartbeat d'un Leader avec un terme supérieur ou égal au sien, il accepte ce Leader et repasse en état Follower.

---

## 5. La réplication du log

### Anatomie d'une entrée de log

Chaque entrée du log contient :
- Un **numéro de terme** (pour détecter les incohérences).
- Un **index** (sa position dans le log).
- La **commande** à exécuter (ex : "set x = 3").

### Cycle de vie d'une commande client

1. **Le client envoie une commande** au Leader (ou à un Follower qui le redirige vers le Leader).
2. **Le Leader ajoute la commande** à la fin de son log — elle est pour l'instant *non validée*.
3. **Le Leader envoie l'entrée** aux Followers via des messages `AppendEntries` (le même message que les heartbeats, mais avec des données).
4. **Les Followers ajoutent l'entrée** à leur propre log et **confirment** au Leader.
5. Dès que le Leader reçoit la confirmation de la **majorité** des serveurs, il **valide** l'entrée (*commit*).
6. Le Leader **applique la commande** à sa machine à états et **répond au client**.
7. Le Leader **informe les Followers** de la validation. Les Followers appliquent la commande à leur tour.

### La règle de cohérence du log

Raft garantit une propriété importante : si deux entrées dans des logs différents ont le même index et le même terme, elles sont identiques, et toutes les entrées qui précèdent sont elles aussi identiques. Cette propriété est maintenue par une vérification à chaque `AppendEntries` : le Leader inclut l'index et le terme de l'entrée précédente, et le Follower refuse le message s'ils ne correspondent pas.

### Les heartbeats

Le Leader envoie des heartbeats périodiquement à tous les Followers, même quand il n'y a pas de nouvelles entrées. Ces heartbeats :
- Signalent que le Leader est toujours vivant (et évitent des élections inutiles).
- Permettent aux Followers en retard de recevoir les entrées manquantes.
- Maintiennent la synchronisation des logs.

---

## 6. La sécurité — ce que Raft garantit

### La propriété de sécurité du Leader

Raft garantit qu'il ne peut jamais y avoir deux Leaders actifs pour le même terme. Comme chaque serveur ne vote qu'une seule fois par terme, et qu'une majorité est nécessaire pour gagner, deux candidats ne peuvent pas tous les deux obtenir la majorité dans le même terme.

### La propriété de complétude du log

Si une entrée est validée dans un terme donné, elle sera présente dans le log de tout futur Leader. Pourquoi ? Parce que pour être élu, un Leader doit obtenir les votes de la majorité — et pour voter pour un candidat, un serveur exige que le candidat ait un log au moins aussi à jour que le sien. Or la majorité des serveurs a forcément connaissance de l'entrée validée (c'est la condition de validation). Donc le futur Leader, pour avoir obtenu la majorité des votes, a forcément cette entrée dans son log.

### La propriété de cohérence des machines à états

Si deux serveurs ont appliqué les mêmes entrées jusqu'à l'index N, ils sont dans le même état. Raft garantit que les logs convergent vers un état identique sur tous les serveurs actifs.

---

## 7. Tolérance aux pannes

### La règle de la majorité

Raft nécessite une **majorité** (quorum) de serveurs actifs pour fonctionner. Dans un cluster de N serveurs, la majorité est ⌊N/2⌋ + 1.

| Taille du cluster | Majorité requise | Pannes tolérées |
|:-----------------:|:----------------:|:---------------:|
| 3 | 2 | 1 |
| 5 | 3 | 2 |
| 7 | 4 | 3 |
| 9 | 5 | 4 |

### Pourquoi des nombres impairs ?

Avec 4 serveurs, la majorité est 3, donc on ne tolère qu'1 panne — autant que 3 serveurs. Le 4ème serveur ne sert à rien en termes de tolérance aux pannes, il ne fait qu'augmenter le coût de coordination. C'est pour ça qu'on déploie toujours Raft avec 3, 5 ou 7 serveurs.

### Ce qui se passe lors d'une panne

**Panne d'un Follower** : rien ne change. Le Leader continue de recevoir des confirmations de la majorité restante et continue à valider des entrées normalement.

**Panne du Leader** : les Followers détectent l'absence de heartbeats. Après le timeout d'élection, une nouvelle élection démarre. En quelques centaines de millisecondes, un nouveau Leader est élu et le service reprend.

**Panne de la majorité** : le cluster s'arrête de prendre de nouvelles décisions. Il ne répond plus aux clients. C'est un choix délibéré — mieux vaut ne rien faire que faire quelque chose d'incorrect.

### Partitionnement réseau (*split brain*)

Un partitionnement réseau, c'est quand le réseau se coupe et que les serveurs forment deux groupes isolés. Raft gère ce cas correctement : seule la partition qui contient la majorité peut élire un Leader et valider des entrées. L'autre partition peut élire un Leader aussi, mais il ne peut pas valider d'entrées (pas de majorité). Quand la partition se résout, le faux Leader de la minorité voit un terme plus élevé, se rend compte qu'un autre Leader existe, et repasse en Follower. Les entrées potentiellement incohérentes de la minorité sont écrasées.

---

## 8. Raft vs Paxos

Paxos, créé par Leslie Lamport dans les années 1980, est l'algorithme de consensus historique. Il est mathématiquement correct mais réputé extrêmement difficile à comprendre et à implémenter.

| | Raft | Paxos |
|---|---|---|
| **Objectif de conception** | Lisibilité | Correction |
| **Architecture** | Leader fort, rôles clairs | Rôles flous, phases multiples |
| **Élection du leader** | Explicite, terme par terme | Implicite, plus complexe |
| **Réplication du log** | Leader vers Followers, directionnel | Plus symétrique, plus difficile à raisonner |
| **Implémentations** | ~100 open source de qualité | Peu d'implémentations fiables connues |
| **Enseignement** | Cours universitaires partout | Rarement enseigné en détail |
| **Production** | Standard de facto depuis 2014 | Utilisé dans des systèmes legacy |

La différence fondamentale : Raft décompose le problème en sous-problèmes indépendants (élection, réplication, sécurité) traités séparément. Paxos fusionne tout, ce qui le rend plus difficile à analyser et à corriger.

---

## 9. Raft dans le monde réel

### etcd

etcd est une base de données clé-valeur distribuée, utilisée comme **stockage central de configuration** par Kubernetes. Kubernetes orchestre les conteneurs dans les clouds modernes (AWS, Google Cloud, Azure…). Toute la configuration d'un cluster Kubernetes — quels services tournent, combien de répliques, quelles ressources — est stockée dans etcd via Raft.

### CockroachDB

CockroachDB est une base de données SQL distribuée et multi-régions. Elle utilise Raft à l'échelle de chaque *shard* (fragment de données) pour garantir que les données restent cohérentes même si un datacenter entier est déconnecté.

### Consul

Consul, développé par HashiCorp, est un outil de **découverte de services et de configuration** pour les architectures microservices. Son catalogue de services (quels serveurs exposent quels ports, leur état de santé) est maintenu par Raft.

### Kafka (mode KRaft)

Apache Kafka, la plateforme de streaming de données la plus utilisée au monde, a migré ses métadonnées de ZooKeeper vers son propre consensus Raft interne, appelé **KRaft** (*Kafka Raft*). Depuis Kafka 3.3, KRaft est le mode recommandé.

### TiKV

TiKV est un moteur de stockage distribué utilisé par TiDB, une base de données SQL cloud-native. Il utilise Raft par groupe de réplication pour garantir la cohérence des données.

### NATS JetStream

NATS est un système de messagerie haute performance. Son module JetStream (persistance des messages) utilise Raft pour la coordination entre nœuds.

---

## 10. Limites de Raft

### Pas de tolérance aux pannes byzantines

Raft suppose que les serveurs tombent en panne de manière honnête — ils s'éteignent, ne répondent plus, mais n'envoient pas de fausses informations. Une **panne byzantine**, c'est quand un serveur envoie délibérément des données incorrectes — comme dans le cas d'un serveur compromis par un attaquant.

Pour les systèmes sans confiance mutuelle (blockchains publiques, systèmes décentralisés), il faut des algorithmes comme PBFT (*Practical Byzantine Fault Tolerance*) ou les protocoles de consensus des blockchains. Une étude de 2023 a confirmé que les blockchains basées sur Raft sont vulnérables aux attaques byzantines.

### Passage à l'échelle limité

Raft fonctionne bien pour des clusters de 3 à 7 serveurs. Au-delà, la coordination devient coûteuse : chaque décision nécessite des échanges avec la majorité, et la latence augmente avec la taille du cluster. Pour des milliers de nœuds, des architectures comme le *sharding* (partitionnement des données entre plusieurs groupes Raft indépendants) sont nécessaires — c'est d'ailleurs ce que fait CockroachDB et TiKV.

### Disponibilité vs cohérence

Raft fait un choix fort en faveur de la **cohérence** au détriment de la disponibilité en cas de partition réseau. Si la majorité des serveurs est inaccessible, le cluster s'arrête complètement plutôt que de risquer une incohérence. C'est le bon choix pour beaucoup d'applications critiques, mais pas pour des systèmes qui privilégient la disponibilité absolue (ex : certains caches distribués).

Ce compromis est décrit par le **théorème CAP** (Consistency, Availability, Partition tolerance) : il est impossible de garantir les trois simultanément. Raft choisit C et P au détriment de A.

### Latence des écritures

Une écriture dans Raft nécessite au minimum un aller-retour réseau entre le Leader et la majorité des Followers avant de répondre au client. Sur des clusters géographiquement distribués (multi-régions), cette latence peut être significative — plusieurs dizaines de millisecondes. Des variantes comme **Multi-Raft** ou **Parallel Raft** cherchent à optimiser ce point.

---

## 11. Glossaire

| Terme | Définition |
|-------|-----------|
| **Algorithme de consensus** | Algorithme permettant à plusieurs machines de s'accorder sur une même valeur malgré des pannes. |
| **AppendEntries** | Message envoyé par le Leader aux Followers pour répliquer des entrées de log ou envoyer des heartbeats. |
| **Candidat** | État transitoire d'un serveur qui tente de devenir Leader lors d'une élection. |
| **Cluster** | Groupe de serveurs travaillant ensemble avec Raft. |
| **Commit / Validation** | Moment où une entrée de log est répliquée sur la majorité et considérée comme définitive. |
| **Consensus** | Processus par lequel plusieurs machines s'accordent sur une même décision. |
| **Follower** | État par défaut d'un serveur Raft. Passif, il répond aux requêtes du Leader. |
| **Heartbeat** | Message périodique envoyé par le Leader pour signaler sa présence et éviter des élections inutiles. |
| **Leader** | Serveur unique qui coordonne le cluster, reçoit les requêtes clients et réplique le log. |
| **Log** | Journal ordonné des commandes reçues et exécutées par le cluster. |
| **Machine à états répliquée** | Modèle où chaque serveur exécute les mêmes commandes dans le même ordre pour rester synchronisé. |
| **Panne byzantine** | Panne où un serveur envoie des informations incorrectes ou malveillantes (non géré par Raft). |
| **Quorum / Majorité** | Nombre minimum de serveurs actifs pour qu'une décision puisse être prise (⌊N/2⌋ + 1). |
| **Raft** | Algorithme de consensus conçu pour la lisibilité. RFC 7748, Ongaro & Ousterhout, 2014. |
| **RequestVote** | Message envoyé par un Candidat pour demander des votes lors d'une élection. |
| **Split brain** | Situation où un réseau partitionné crée deux groupes qui croient chacun être indépendants. |
| **Système distribué** | Ensemble de machines indépendantes qui collaborent et communiquent via un réseau. |
| **Terme** | Période numérotée dans Raft, commençant par une élection. Compteur monotone croissant. |
| **Théorème CAP** | Il est impossible pour un système distribué de garantir simultanément cohérence, disponibilité et tolérance aux partitions. |
| **Théorème FLP** | Preuve qu'un consensus déterministe est impossible dans un système asynchrone avec au moins une panne possible. |
| **Timeout d'élection** | Durée aléatoire après laquelle un Follower qui n'entend plus le Leader déclenche une élection. |

---

*Sources : Diego Ongaro & John Ousterhout, "In Search of an Understandable Consensus Algorithm" (USENIX ATC 2014) · raft.github.io · Wikipedia · GeeksforGeeks · FreeCodeCamp*
