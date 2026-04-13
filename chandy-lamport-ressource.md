# Chandy-Lamport — Ressource théorique

> Document de référence pour l'exposé. Public : étudiants en 3ᵉ année de licence informatique connaissant les bases des systèmes distribués.

---

## 1. Pourquoi un algorithme de snapshot distribué ?

### 1.1 Le problème fondamental

Dans un système distribué, il n'y a **pas d'horloge globale** ni de mémoire partagée. Chaque processus a son propre état local et communique avec les autres par **envoi de messages** à travers des canaux.

Question : comment capturer l'état global du système à un instant donné, alors qu'il n'existe pas d'« instant donné » partagé ?

On ne peut pas simplement demander à chaque processus « donne-moi ton état maintenant », car :
- le message de demande met du temps à arriver ;
- pendant ce temps, les processus continuent d'évoluer ;
- des messages peuvent être en transit dans les canaux.

### 1.2 Les applications concrètes

Un snapshot cohérent de l'état global sert à :

- **Détection de terminaison** : savoir si un calcul distribué est fini (plus aucun message en transit, tous les processus inactifs).
- **Détection d'interblocage** : vérifier si un cycle d'attente existe entre processus.
- **Tolérance aux pannes (checkpointing)** : sauvegarder un état cohérent pour pouvoir y revenir après un crash.
- **Débogage** : inspecter l'état d'un système distribué sans l'arrêter.

### 1.3 Contexte historique

L'algorithme est publié en **1985** par **K. Mani Chandy** (Caltech) et **Leslie Lamport** (alors chez SRI International, futur prix Turing 2013). L'article s'intitule *"Distributed Snapshots: Determining Global States of Distributed Systems"*, publié dans ACM Transactions on Computer Systems.

Lamport est aussi l'auteur des horloges logiques (1978), de l'algorithme de la boulangerie (exclusion mutuelle), et du consensus Paxos.

---

## 2. Modèle du système

L'algorithme repose sur des hypothèses précises. Si une seule n'est pas respectée, l'algorithme ne fonctionne pas.

### 2.1 Hypothèses

1. **Canaux FIFO** : les messages arrivent dans l'ordre dans lequel ils ont été envoyés. C'est l'hypothèse la plus importante.
2. **Canaux fiables** : pas de perte, pas de duplication, pas de corruption de messages.
3. **Graphe fortement connexe** : tout processus peut atteindre tout autre processus (directement ou indirectement).
4. **Canaux unidirectionnels** : entre deux processus P et Q, il existe un canal P→Q et un canal Q→P (deux canaux distincts).

### 2.2 État du système

L'**état global** du système à un instant donné est composé de :

- **L'état local** de chaque processus (variables, mémoire, compteurs, etc.)
- **L'état de chaque canal** : l'ensemble des messages qui sont en transit (envoyés mais pas encore reçus).

Un snapshot doit capturer **les deux** : les états locaux ET les messages en transit.

### 2.3 Notion de coupure cohérente (consistent cut)

Un **cut** est un ensemble d'événements, un par processus, qui représente un « instant » pour chaque processus.

Un **cut cohérent** (consistent cut) vérifie : si un événement de réception est dans le cut, alors l'événement d'envoi correspondant est aussi dans le cut. Autrement dit : on ne peut pas observer la réception d'un message sans que son envoi ait eu lieu.

L'algorithme de Chandy-Lamport produit toujours un **cut cohérent**, même si le snapshot n'a jamais réellement existé comme état instantané.

---

## 3. L'algorithme de Chandy-Lamport

### 3.1 L'outil clé : le message marqueur

L'algorithme utilise un message spécial appelé **marqueur** (marker). Ce n'est pas un message applicatif — c'est un message de contrôle qui sert à séparer les messages « avant le snapshot » de ceux « après le snapshot ».

Le marqueur joue le rôle de **frontière temporelle** sur chaque canal.

### 3.2 Initiation

N'importe quel processus peut décider d'initier un snapshot. Appelons-le **P_i**.

**Étape 1 — Initiation par P_i** :
1. P_i **enregistre son propre état local**.
2. P_i envoie un **marqueur** sur **chacun de ses canaux sortants**, avant d'envoyer tout autre message.

### 3.3 Réception d'un marqueur

Quand un processus **P_j** reçoit un marqueur sur un canal **C** venant de **P_k** :

**Cas 1 — Premier marqueur reçu par P_j** (P_j n'a pas encore enregistré son état) :
1. P_j **enregistre son état local**.
2. P_j note que l'état du canal C (celui par lequel le marqueur est arrivé) est **vide** (aucun message en transit sur ce canal au moment du snapshot).
3. P_j commence à **enregistrer les messages** arrivant sur **tous les autres canaux entrants**.
4. P_j envoie un **marqueur** sur **chacun de ses canaux sortants**.

**Cas 2 — Marqueur déjà reçu auparavant** (P_j a déjà enregistré son état) :
1. P_j **arrête d'enregistrer** les messages sur le canal C.
2. L'ensemble des messages enregistrés sur C depuis l'enregistrement de l'état local de P_j constitue **l'état du canal C** dans le snapshot.

### 3.4 Terminaison

L'algorithme se termine quand **chaque processus** a reçu un marqueur sur **chacun de ses canaux entrants**. À ce moment, chaque processus a :
- Son état local enregistré
- L'état de chacun de ses canaux entrants enregistré

L'union de ces informations forme le **snapshot global**.

### 3.5 Résumé en pseudo-code

```
INITIATION par P_i :
    enregistrer état_local(P_i)
    pour chaque canal sortant C_out :
        envoyer MARQUEUR sur C_out

RÉCEPTION d'un MARQUEUR sur canal C par P_j :
    si P_j n'a pas encore enregistré son état :
        enregistrer état_local(P_j)
        état(C) ← vide
        pour chaque canal entrant C_in ≠ C :
            commencer à enregistrer les messages sur C_in
        pour chaque canal sortant C_out :
            envoyer MARQUEUR sur C_out
    sinon :
        état(C) ← messages enregistrés sur C depuis l'enregistrement de P_j
        arrêter d'enregistrer sur C
```

---

## 4. Exemple détaillé

### 4.1 Configuration

Trois processus : **P1**, **P2**, **P3**, reliés en triangle (chacun peut envoyer à chacun).

État initial :
- P1 : solde = 500€
- P2 : solde = 300€
- P3 : solde = 200€

Total dans le système : 1000€ (invariant — pas de création ni de destruction d'argent).

### 4.2 Déroulement

1. **P1 envoie 100€ à P2** (message M1 en transit sur le canal P1→P2).
   - État de P1 après envoi : 400€

2. **P1 initie le snapshot** :
   - P1 enregistre son état : **400€**
   - P1 envoie un marqueur à P2 et à P3

3. **P2 reçoit M1 (les 100€)** avant le marqueur (car FIFO, M1 a été envoyé avant le marqueur) :
   - État de P2 : 400€

4. **P2 reçoit le marqueur de P1** (premier marqueur pour P2) :
   - P2 enregistre son état : **400€**
   - Canal P1→P2 : **vide** (le marqueur est arrivé, tout ce qui était avant a été reçu)
   - P2 commence à enregistrer les messages sur le canal P3→P2
   - P2 envoie un marqueur à P1 et P3

5. **P3 envoie 50€ à P2** (message M2 en transit sur P3→P2).
   - État de P3 après envoi : 150€

6. **P3 reçoit le marqueur de P1** (premier marqueur pour P3) :
   - P3 enregistre son état : **150€**
   - Canal P1→P3 : **vide**
   - P3 commence à enregistrer sur P2→P3
   - P3 envoie un marqueur à P1 et P2

7. **P2 reçoit M2 (50€ de P3)** — ce message est arrivé AVANT le marqueur de P3 sur le canal P3→P2. P2 l'enregistre.

8. **P2 reçoit le marqueur de P3** :
   - Canal P3→P2 : **{M2 = 50€}** (les messages enregistrés depuis l'étape 4)

9. **P1 reçoit le marqueur de P2** :
   - Canal P2→P1 : **vide**

10. **P1 reçoit le marqueur de P3** :
    - Canal P3→P1 : **vide**

11. **P3 reçoit le marqueur de P2** :
    - Canal P2→P3 : **vide**

### 4.3 Résultat du snapshot

| Composant | État enregistré |
|---|---|
| P1 | 400€ |
| P2 | 400€ |
| P3 | 150€ |
| Canal P3→P2 | 50€ en transit |
| Tous les autres canaux | vides |

**Total** : 400 + 400 + 150 + 50 = **1000€** — l'invariant est respecté.

Le message M2 (50€ de P3 vers P2) a été envoyé avant que P3 enregistre son état (P3 avait déjà 150€), mais reçu après que P2 a enregistré le sien (P2 avait 400€). Il apparaît donc dans l'état du canal P3→P2. Rien n'est perdu ni dupliqué.

---

## 5. Pourquoi ça marche ?

### 5.1 Le rôle de FIFO

L'hypothèse FIFO est essentielle. Elle garantit que sur chaque canal, le marqueur arrive **après** tous les messages envoyés avant lui et **avant** tous les messages envoyés après lui.

Sans FIFO, un message envoyé avant le snapshot pourrait arriver après le marqueur, et ne serait pas comptabilisé — on aurait un état incohérent.

### 5.2 Cohérence du snapshot

Le snapshot produit est un **cut cohérent** : si un message est comptabilisé comme reçu (inclus dans l'état du récepteur), alors il est aussi comptabilisé comme envoyé (absent de l'état de l'émetteur).

Inversement, si un message est en transit (dans l'état d'un canal), il a été envoyé (l'émetteur a déduit la somme de son état) mais pas encore reçu (le récepteur ne l'a pas dans son état).

### 5.3 L'état capturé est-il « réel » ?

Pas forcément. Le snapshot peut capturer un état global qui **n'a jamais existé à un instant donné**. Mais il est **atteignable** : c'est un état par lequel le système aurait pu passer. C'est suffisant pour la détection de propriétés stables (interblocage, terminaison).

---

## 6. Complexité

| Métrique | Valeur |
|---|---|
| **Messages marqueurs** | O(E) où E = nombre de canaux (arêtes du graphe) |
| **Temps** | O(D) où D = diamètre du graphe (en nombre de sauts) |
| **Espace** | O(M) où M = nombre total de messages en transit au moment du snapshot |

L'algorithme est **léger** : il n'envoie qu'un marqueur par canal, et ne bloque aucun processus.

---

## 7. Limites et extensions

### 7.1 Limites

- **FIFO obligatoire** : si les canaux ne sont pas FIFO (ex : UDP), l'algorithme est incorrect. Solution : ajouter des numéros de séquence pour recréer l'ordre.
- **Pas de tolérance aux pannes** : si un processus crash pendant le snapshot, le snapshot est incomplet.
- **Canaux fiables** : si un marqueur est perdu, un processus ne saura jamais qu'il doit enregistrer son état.

### 7.2 Extensions

- **Lai-Yang (1987)** : supprime l'hypothèse FIFO en coloriant les messages (pré-snapshot vs post-snapshot).
- **Mattern (1993)** : utilise des horloges vectorielles pour construire des snapshots cohérents sans hypothèse FIFO.
- **Snapshots asynchrones dans Flink** : Apache Flink implémente une variante de Chandy-Lamport pour le checkpointing de flux de données distribués (ABS — Asynchronous Barrier Snapshotting).

---

## 8. Pour aller plus loin

- **Chandy & Lamport**, *"Distributed Snapshots: Determining Global States of Distributed Systems"*, ACM TOCS, 1985
- **Coulouris, Dollimore & Kindberg**, *Distributed Systems: Concepts and Design*, chapitre 14
- **Lamport**, *"Time, Clocks, and the Ordering of Events in a Distributed System"*, 1978
- **Carbone et al.**, *"Lightweight Asynchronous Snapshots for Distributed Dataflows"*, 2015 (Flink)
- **RFC 3286** — An Introduction to the Stream Control Transmission Protocol (SCTP), pour contexte sur les canaux FIFO
