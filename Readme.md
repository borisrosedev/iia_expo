# Cours — Horloges logiques de Lamport

<aside>
🕐

**RSX102 — Technologies pour les applications en réseau**

Exposé présenté par Elouan Tusseau

</aside>

---

## Introduction

Imaginez deux ingénieurs qui travaillent à distance sur le même fichier. L'un enregistre une modification à 14h00 selon son ordinateur, l'autre en enregistre une autre à 13h58 selon le sien. Quelle modification est arrivée en premier ? Si les horloges des deux machines ne sont pas parfaitement synchronisées — ce qui est toujours le cas dans un réseau — **il est impossible de répondre à cette question avec certitude**.

C'est précisément le défi fondamental des **systèmes distribués** : plusieurs machines communiquent via un réseau, chacune avec sa propre horloge, sans qu'aucune ne fasse autorité sur les autres. Les délais réseau sont variables et imprévisibles. Dans ce contexte, comment établir un ordre fiable entre les événements ?

En 1978, le mathématicien et informaticien **Leslie Lamport** publie une réponse élégante à ce problème : les **horloges logiques**. L'idée est de remplacer l'heure physique par un simple compteur, suffisant pour capturer ce qui compte vraiment — **la causalité**.

Nous verrons dans cet exposé pourquoi les horloges physiques sont insuffisantes, comment fonctionne l'algorithme de Lamport, ce qu'il garantit et ce qu'il ne garantit pas, et enfin comment il s'applique dans les systèmes modernes.

---

## I. Le problème : l'impossibilité d'une horloge globale

### A. Pourquoi les horloges physiques ne suffisent pas

Dans un système à une seule machine, ordonner les événements est trivial : on lit l'horloge système. Mais dans un **système distribué**, cette approche s'effondre pour deux raisons :

1. **La dérive des horloges** — chaque machine a sa propre horloge interne qui s'écarte progressivement des autres, même avec des protocoles de synchronisation comme NTP.
2. **La variabilité du réseau** — un message envoyé à l'instant *t* peut arriver après un délai indéterminé. Il n'existe pas de "temps de transit" fixe et connu à l'avance.

> **Problème concret :** La machine A envoie un message à t = 10s selon son horloge. La machine B le reçoit à t = 9s selon la sienne. Qui a raison ? Dans quel ordre les événements se sont-ils vraiment passés ?
> 

Il n'y a pas de réponse basée sur l'heure physique. Pourtant, sans ordre, la coordination devient impossible : deux processus pourraient accéder simultanément à une ressource critique, des transactions pourraient s'appliquer dans le mauvais ordre, des logs deviendraient impossibles à reconstruire.

### B. Ce qui compte vraiment : la causalité

Lamport observe que l'on n'a pas réellement besoin de connaître *l'heure exacte* d'un événement. Ce qui importe, c'est de savoir **si un événement A a pu influencer un événement B** — c'est-à-dire si A précède causalement B.

On note cette relation : **A → B** ("A précède causalement B"), qui se lit : A s'est produit avant B, et B a potentiellement connaissance de A.

Cette relation de causalité suffit pour coordonner des systèmes distribués. L'idée de Lamport est de la capturer avec un simple compteur entier.

---

## II. L'algorithme des horloges logiques de Lamport

### A. Principe général

Chaque processus maintient un compteur entier appelé **LC** (Logical Clock), initialisé à **0**. Ce compteur est mis à jour selon trois règles simples, selon la nature de chaque événement.

### B. Les trois règles

**Règle 1 — Événement local**

Quand un processus réalise une action interne (calcul, écriture locale…) :

```
LC = LC + 1
```

**Règle 2 — Envoi d'un message**

Avant d'envoyer un message, le processus incrémente son compteur et joint la valeur au message :

```
LC = LC + 1
message.timestamp = LC
```

**Règle 3 — Réception d'un message**

À la réception d'un message portant le timestamp `LC_message` :

```
LC = max(LC_local, LC_message) + 1
```

> Le `max` garantit que la réception a toujours une valeur **strictement supérieure** à l'envoi — la causalité est ainsi préservée mécaniquement.
> 

### C. Exemple pas à pas

On considère 3 processus : **P1**, **P2**, **P3**. Toutes les horloges démarrent à **0**.

| Étape | Événement | Processus | Calcul | LC résultant |
| --- | --- | --- | --- | --- |
| 1 | Événement local | P1 | 0 + 1 | **1** |
| 2 | Envoi à P2 | P1 | 1 + 1 | **2** |
| 3 | Réception de P1 | P2 | max(0, 2) + 1 | **3** |
| 4 | Envoi à P3 | P2 | 3 + 1 | **4** |
| 5 | Réception de P2 | P3 | max(0, 4) + 1 | **5** |

On peut lire l'ordre global : **1 → 2 → 3 → 4 → 5**. La causalité est parfaitement respectée, sans aucune horloge physique commune.

---

## III. Ce que l'algorithme garantit — et ses limites

### A. La garantie fondamentale

L'algorithme de Lamport offre une propriété précise et prouvable :

> ✅ **Si A → B (A cause B), alors LC(A) < LC(B)**
> 

Autrement dit : si un événement A précède causalement B, son timestamp sera toujours strictement inférieur. L'ordre causal est donc **toujours respecté** dans la numérotation.

### B. Ce qu'il ne peut pas faire

Mais la réciproque n'est pas vraie :

> ⚠️ **Si LC(A) < LC(B), on ne peut PAS conclure que A cause B.**
> 

Deux événements sur des processus distincts peuvent avoir des timestamps ordonnés sans aucun lien causal. On dit alors qu'ils sont **concurrents**. L'horloge de Lamport ne permet pas de les distinguer d'événements causalement liés.

**Exemple illustré :**

```
P1 :  LC=1 (local)          LC=3 (reçoit de P2)
P2 :  LC=2 (envoie à P1)
P3 :  LC=1 (local, indépendant de tout)
```

Ici, P3 (LC=1) et P2 (LC=2) semblent ordonnés — mais P3 n'a aucun lien avec P2. Ils sont **concurrents**, et Lamport ne permet pas de le détecter.

### C. La solution pour aller plus loin : les horloges vectorielles

Pour détecter la concurrence, on utilise les **horloges vectorielles** (Mattern & Fidge, 1988). Chaque processus maintient un vecteur `V[1..n]` où `V[i]` représente le nombre d'événements connus de P_i.

| Propriété | Horloge de Lamport | Horloge vectorielle |
| --- | --- | --- |
| A → B ⇒ LC(A) < LC(B) | ✅ | ✅ |
| LC(A) < LC(B) ⇒ A → B | ❌ | ✅ |
| Détection de la concurrence | ❌ | ✅ |
| Coût mémoire | O(1) | O(n) |

> 💡 Les horloges de Lamport sont suffisantes pour **ordonner** les événements. Les horloges vectorielles sont nécessaires quand on veut aussi **détecter la concurrence**.
> 

---

## IV. Applications et prolongements

### A. Cas d'usage concrets

L'algorithme de Lamport n'est pas qu'un outil théorique : il est à la base de nombreux mécanismes des systèmes distribués modernes.

**Exclusion mutuelle — Ricart-Agrawala**

C'est l'application la plus emblématique. Quand un processus P_i veut entrer en section critique :

1. Il envoie une **requête horodatée** `(LC, i)` à tous les autres
2. Un processus P_j accorde la permission si sa propre requête a un timestamp plus grand
3. L'ordre total `(LC, i)` — timestamp d'abord, puis ID en cas d'égalité — garantit l'**absence de famine**

Sans horloge logique, il serait impossible d'ordonner ces requêtes de façon déterministe.

**Snapshot global — Chandy-Lamport**

L'algorithme de snapshot de Chandy-Lamport (1985) repose sur la même notion de causalité pour capturer un état global cohérent d'un système distribué, sans horloge physique commune.

**Autres utilisations**

| Contexte | Rôle des horloges logiques |
| --- | --- |
| Bases de données distribuées (Cassandra, DynamoDB) | Ordonnancement des transactions, résolution de conflits |
| Systèmes de fichiers distribués (Ceph, GFS) | Détection et résolution des écritures concurrentes |
| Débogage distribué | Reconstruction d'une timeline cohérente depuis des logs multi-sources |
| Consensus (Raft, Paxos) | Ordonnancement des entrées du journal répliqué |

### B. Contribution aux propriétés fondamentales

Les horloges logiques contribuent aussi aux grandes propriétés des algorithmes distribués vues en cours :

- **Sûreté** — l'ordonnancement déterministe évite les états incohérents dus à des événements mal ordonnés
- **Absence de famine** — dans Ricart-Agrawala, le timestamp Lamport garantit que chaque requête sera honorée en temps fini
- **Diffusion causale** — les timestamps permettent de livrer les messages dans un ordre cohérent avec la causalité

---

## Conclusion

Les horloges logiques de Lamport répondent à un problème fondamental : **comment ordonner des événements dans un système où aucune horloge globale n'existe ?** La solution est d'une élégance remarquable — un simple compteur entier, trois règles de mise à jour, et une propriété garantie : l'ordre causal est toujours respecté.

Bien sûr, cette solution a ses limites. Elle ne permet pas de détecter la concurrence, ce que font les horloges vectorielles au prix d'un coût mémoire plus élevé. Mais dans de nombreux contextes — exclusion mutuelle, logs distribués, ordonnancement de transactions — l'horloge de Lamport est **suffisante et très efficace**.

Publiée en 1978, cette contribution reste aujourd'hui au cœur des systèmes distribués modernes : Cassandra, DynamoDB, Raft, ou encore les algorithmes de snapshot l'utilisent tous, directement ou indirectement.

> **À retenir** : LC(A) < LC(B) si A → B. Mais l'inverse n'est pas vrai. Et c'est précisément cette nuance qui distingue les horloges logiques des horloges vectorielles.
> 

---

## Exercice — À vous de jouer

On considère **3 processus : P1, P2, P3**. Toutes les horloges démarrent à **0**.

**Suite d'événements :**

1. P2 réalise un événement local
2. P3 réalise un événement local
3. P2 envoie un message à P1
4. P1 reçoit le message de P2
5. P3 envoie un message à P2
6. P2 reçoit le message de P3
7. P1 réalise un événement local

**Question : Calculer la valeur de l'horloge logique LC après chaque événement.**

- ✅ Correction
    
    
    | Étape | Événement | Processus | Calcul | LC |
    | --- | --- | --- | --- | --- |
    | 1 | Événement local | P2 | 0 + 1 | **1** |
    | 2 | Événement local | P3 | 0 + 1 | **1** |
    | 3 | P2 envoie à P1 | P2 | 1 + 1 | **2** |
    | 4 | P1 reçoit de P2 (LC=2) | P1 | max(0, 2) + 1 | **3** |
    | 5 | P3 envoie à P2 (LC=1) | P3 | 1 + 1 | **2** |
    | 6 | P2 reçoit de P3 (LC=2) | P2 | max(2, 2) + 1 | **3** |
    | 7 | Événement local | P1 | 3 + 1 | **4** |
    
    **État final :** P1 = **4** · P2 = **3** · P3 = **2**
    

---

## Sources et références

- Leslie Lamport, *"Time, Clocks, and the Ordering of Events in a Distributed System"*, Communications of the ACM, 1978
- Mattern & Fidge, horloges vectorielles, 1988
- Ricart & Agrawala, algorithme d'exclusion mutuelle, 1981
- Chandy & Lamport, algorithme de snapshot global, 1985
