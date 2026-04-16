# Paxos — Algorithme de consensus distribué

## Sommaire

1. [Contexte et problématique](#1-contexte-et-problématique)
2. [Historique et origine](#2-historique-et-origine)
3. [Qu'est-ce que le consensus distribué ?](#3-quest-ce-que-le-consensus-distribué-)
4. [Les rôles dans Paxos](#4-les-rôles-dans-paxos)
5. [Les phases de l'algorithme](#5-les-phases-de-lalgorithme)
6. [Propriétés garanties](#6-propriétés-garanties)
7. [Variantes de Paxos](#7-variantes-de-paxos)
8. [Paxos dans la vraie vie](#8-paxos-dans-la-vraie-vie)
9. [Paxos vs Raft](#9-paxos-vs-raft)
10. [Limites de Paxos](#10-limites-de-paxos)
11. [Exercice](#11-exercice)
12. [Paxos en uhe phrase](#12-paxos-en-une-phrase)
13. [Glossaire](#13-glossaire)

---

## 1. Contexte et problématique

Dans un système distribué, plusieurs nœuds (serveurs) coopèrent pour stocker et traiter des données. Mais que se passe-t-il si :
- Un serveur tombe en panne ?
- Un message réseau est perdu ou arrive en retard ?
- Plusieurs serveurs reçoivent des valeurs différentes ?

> **Problème fondamental :** comment faire en sorte que tous les nœuds d'un réseau **se mettent d'accord sur une même valeur**, malgré des pannes et des délais imprévisibles ?

C'est le **problème du consensus distribué**, et Paxos est l'une des solutions les plus importantes qui y répond.

---

## 2. Historique et origine

- **1989 :** Leslie Lamport conçoit l'algorithme Paxos. Initialement, il cherchait à démontrer qu'un tel algorithme *ne pouvait pas exister* — il a trouvé le contraire.
- **1990 :** Soumission du papier original *"The Part-Time Parliament"*, mais il est jugé trop difficile à comprendre et rejeté par les reviewers.
- **1998 :** Publication officielle dans un journal scientifique, 8 ans après la soumission initiale.
- **2001 :** Lamport publie *"Paxos Made Simple"* pour clarifier l'algorithme.
- **Aujourd'hui :** Paxos est utilisé par Google, Amazon, Microsoft, Apache et bien d'autres.

> Le nom vient de l'île grecque de **Paxos**, dont le parlement fictif devait fonctionner même si les législateurs "entraient et sortaient constamment de la chambre" — une métaphore parfaite pour les nœuds défaillants.

---

## 3. Qu'est-ce que le consensus distribué ?

Le consensus distribué permet à un ensemble de processus de **s'accorder sur une valeur unique**, même dans un environnement peu fiable.

### Trois propriétés fondamentales

| Propriété | Signification |
|-----------|--------------|
| **Agreement** | Tous les nœuds sains convergent vers la **même valeur** |
| **Validity** | La valeur choisie doit avoir été **proposée** par l'un des nœuds |
| **Liveness** | Si suffisamment de nœuds fonctionnent, une décision **finira toujours** par être prise |

### Hypothèses de Paxos

- Le réseau est **asynchrone** : les messages peuvent arriver dans n'importe quel ordre, ou pas du tout.
- Les nœuds peuvent **planter** (*crash failures*), mais ils ne mentent pas (*pas de fautes byzantines*).
- Les messages ne sont **pas corrompus** en transit.

---

## 4. Les rôles dans Paxos

Paxos définit trois rôles distincts. Un même nœud peut en assumer plusieurs.

### Proposer (Proposant)
- Suggère une valeur à faire accepter par le système.
- Initie le protocole en choisissant un numéro de proposition unique.

### Acceptor (Acceptant)
- Vote sur les valeurs proposées.
- Garantit la majorité : la valeur choisie doit recueillir l'accord d'une **majorité** d'acceptors.
- Maintient un état persistant (numéros de promesses, valeurs acceptées).

### Learner (Apprenant)
- Apprend la valeur finalement choisie, une fois le consensus atteint.
- Ne participe pas aux votes.

```
        [Proposer]
           |
     Prépare / Propose
           |
    [Acceptor 1] [Acceptor 2] [Acceptor 3]
           \         |         /
            \   Majorité     /
             \    atteinte  /
              \            /
             [Learner(s)]
```

---

## 5. Les phases de l'algorithme

### Phase 1 — Prepare (Préparer)

**1a. Le Proposer → envoie `Prepare(n)`**
- Le proposer choisit un numéro de proposition `n` unique et supérieur à tout ce qu'il a vu.
- Il envoie un message `Prepare(n)` à une majorité d'acceptors.

**1b. L'Acceptor → répond `Promise(n, [valeur_déjà_acceptée])`**
- Si `n` est le plus grand numéro de proposition vu jusqu'ici, l'acceptor **promet** de ne plus accepter de proposition numérotée `< n`.
- S'il a déjà accepté une valeur auparavant, il la renvoie.

---

### Phase 2 — Accept (Accepter)

**2a. Le Proposer → envoie `Accept(n, v)`**
- Si le proposer reçoit des promesses d'une majorité d'acceptors, il envoie `Accept(n, v)` où :
  - `v` = la valeur déjà acceptée de la plus haute numérotation, sinon **sa propre valeur**.

**2b. L'Acceptor → accepte ou refuse**
- L'acceptor accepte si `n` est encore le plus grand numéro promis.
- Il notifie le(s) learner(s) de son acceptation.

---

### Phase 3 — Learn (Apprendre)

- Quand une majorité d'acceptors ont accepté la même valeur, le **consensus est atteint**.
- Les learners sont informés de la valeur choisie.

### Schéma complet

```
Proposer          Acceptor 1    Acceptor 2    Acceptor 3
   |                  |              |              |
   |---Prepare(n)---->|              |              |
   |---Prepare(n)---->|              |              |
   |---Prepare(n)---->|              |              |
   |                  |              |              |
   |<--Promise(n,-)---|              |              |
   |<--Promise(n,-)---|              |              |
   |     (majorité reçue)            |              |
   |                  |              |              |
   |---Accept(n,v)--->|              |              |
   |---Accept(n,v)--->|              |              |
   |---Accept(n,v)--->|              |              |
   |                  |              |              |
   |              [Learner notifié : consensus sur v]
```

---

## 6. Propriétés garanties

### Sécurité (Safety)
- **Une seule valeur** peut être choisie (pas de divergence).
- Cette valeur doit avoir été **proposée** par un nœud du système.
- Une valeur apprise est **définitivement** la valeur choisie.

### Vivacité (Liveness)
- Si la majorité des nœuds fonctionnent et peuvent communiquer, le consensus **finira par être atteint**.

> ⚠️ **Limite théorique :** En cas de compétition entre deux proposers (chacun incrémentant `n` indéfiniment), un **livelock** peut survenir. La solution : élire un **leader unique** (Multi-Paxos).

---

## 7. Variantes de Paxos

| Variante | Description |
|----------|-------------|
| **Classic Paxos** | Version originale : Prepare → Promise → Accept → Learn |
| **Multi-Paxos** | Un leader élu évite la phase Prepare pour les instances successives |
| **Fast Paxos** | Les proposers envoient directement aux acceptors (moins de tours) |
| **Cheap Paxos** | Réduit le nombre de nœuds actifs nécessaires |
| **Byzantine Paxos** | Supporte les nœuds qui mentent (fautes byzantines) |
| **Egalitarian Paxos** | Tout nœud peut proposer sans leader central |

---

## 8. Paxos dans la vraie vie

Paxos (ou ses dérivés) est utilisé dans de nombreux systèmes critiques :

| Système | Usage |
|---------|-------|
| **Google Chubby** | Service de verrouillage distribué, utilisé par Bigtable |
| **Google Spanner** | Base de données SQL distribuée globalement |
| **Amazon DynamoDB** | Élection de leader et consensus |
| **Apache ZooKeeper** | Coordination de services distribués (ZAB, inspiré de Paxos) |
| **Apache Cassandra** | Transactions légères (*Lightweight Transactions*) |
| **Microsoft Autopilot** | Gestion de clusters Bing |
| **Amazon ECS** | Maintien d'une vue cohérente de l'état du cluster |

---

## 9. Paxos vs Raft

Raft a été créé en 2013 comme alternative plus simple à Paxos.

| Critère | Paxos | Raft |
|---------|-------|------|
| Compréhensibilité | Difficile | Facile |
| Leader | Optionnel (Multi-Paxos) | Obligatoire |
| Élection de leader | Non intégrée à la base | Intégrée (timeouts aléatoires) |
| Cas d'usage | Systèmes critiques, recherche | Systèmes modernes (etcd, Consul) |
| Flexibilité | Haute (nombreuses variantes) | Moins flexible |

> Raft = "In Search of an Understandable Consensus Algorithm" (Diego Ongaro, John Ousterhout, 2013)

---

## 10. Limites de Paxos

- **Complexité d'implémentation** élevée : même les experts font des erreurs.
- **Livelock possible** en cas de compétition entre proposers.
- **Pas de fautes byzantines** gérées dans la version classique.
- **Pas de gestion native du log** (nécessite Multi-Paxos).
- **Pas supporté nativement** par les navigateurs web ni facilement traversable via NAT.

---

## 11. Exercice

### Testez vos connaissances sur Paxos

**Question 1 :** Quel rôle dans Paxos *vote* sur les valeurs proposées ?

Acceptor ✅

**Question 2 :** Combien d'acceptors doivent accepter une valeur pour qu'elle soit choisie ?

Une **majorité** ✅

**Question 3 :** Qu'est-ce qu'un livelock dans Paxos ?

Une situation où deux proposers s'empêchent mutuellement d'aboutir ✅

**Question 4 :** Quelle variante de Paxos élit un leader pour éviter la phase Prepare à chaque instance ?

Multi-Paxos ✅

**Question 5 :** Parmi ces systèmes, lequel utilise Paxos pour la cohérence distribuée ?

Google Chubby ✅

---

## 12. Paxos en une phrase 

En conclusion, on peut dire que « Le handshake TLS est la phase de mise en confiance cryptographique
entre un client et un serveur. »

De même, Paxos est la phase de mise en confiance entre des nœuds distribués — garantissant qu'ils s'accordent tous sur la même vérité.

## 13. Glossaire

| Terme | Définition |
|-------|-----------|
| **Consensus distribué** | Mécanisme permettant à des nœuds d'un réseau de s'accorder sur une même valeur, même en présence de pannes. |
| **Proposer** | Rôle Paxos : nœud qui propose une valeur à faire adopter par le système. |
| **Acceptor** | Rôle Paxos : nœud qui vote sur les valeurs proposées et maintient l'état du consensus. |
| **Learner** | Rôle Paxos : nœud qui apprend la valeur finalement choisie, sans participer aux votes. |
| **Majorité (quorum)** | Nombre minimal de nœuds devant s'accorder pour qu'une décision soit valide : ⌊N/2⌋ + 1. |
| **Numéro de proposition (n)** | Identifiant unique et croissant que le proposer attribue à chaque tentative. |
| **Prepare** | Phase 1 de Paxos : le proposer demande aux acceptors de promettre de ne plus accepter de proposition plus ancienne. |
| **Promise** | Réponse d'un acceptor au Prepare : il s'engage à ignorer toute proposition < n. |
| **Accept** | Phase 2 de Paxos : le proposer demande aux acceptors d'accepter une valeur concrète. |
| **Learn** | Phase 3 : les learners sont informés de la valeur ayant obtenu la majorité. |
| **Livelock** | Situation où deux proposers se bloquent mutuellement indéfiniment en incrémentant leur numéro de proposition. |
| **Multi-Paxos** | Extension de Paxos avec un leader élu pour éviter la phase Prepare dans les instances successives. |
| **Safety (Sécurité)** | Propriété garantissant qu'une seule valeur est choisie et qu'elle ne change pas. |
| **Liveness (Vivacité)** | Propriété garantissant que le système finit toujours par prendre une décision. |
| **Crash failure** | Type de panne où un nœud s'arrête et ne répond plus (sans comportement malveillant). |
| **Byzantine failure** | Panne où un nœud peut se comporter de manière arbitraire ou malveillante. |
| **Nœud / Node** | Machine ou processus participant au système distribué. |
| **Asynchrone** | Qualifie un réseau où les délais de transmission sont imprévisibles et non bornés. |
| **Raft** | Algorithme de consensus alternatif à Paxos, conçu pour être plus simple à comprendre et à implémenter. |
| **State machine replication** | Technique consistant à répliquer un état (ex : base de données) sur plusieurs nœuds via un algorithme de consensus. |
| **Google Chubby** | Service de verrouillage distribué de Google, utilisant Paxos pour maintenir la cohérence entre répliques. |
| **Quorum** | Sous-ensemble minimal de nœuds nécessaire pour prendre une décision valide dans un système distribué. |
