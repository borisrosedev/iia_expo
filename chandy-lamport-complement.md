# Chandy-Lamport — Document complémentaire

---

## Mémo rapide

| Étape | Qui | Action |
|---|---|---|
| **Initiation** | P_initiateur | Enregistre son état local, envoie un marqueur sur chaque canal sortant |
| **1er marqueur reçu** | P_j | Enregistre état local, canal du marqueur = vide, enregistre les messages sur les autres canaux, envoie marqueurs |
| **Marqueur suivant sur canal C** | P_j | Arrête d'enregistrer sur C, messages enregistrés = état du canal C |
| **Terminaison** | Tous | Chaque processus a reçu un marqueur sur chaque canal entrant |

**Snapshot = états locaux + états des canaux (messages en transit)**

**Hypothèses requises** : canaux FIFO, fiables, graphe fortement connexe.

---

## Glossaire — Définitions des termes clés

**Canal (Channel)** — Lien de communication unidirectionnel entre deux processus. Les messages y transitent dans l'ordre FIFO.

**Canal FIFO** — Canal où les messages arrivent dans l'ordre exact d'envoi. Hypothèse centrale de Chandy-Lamport. Sans elle, l'algorithme est incorrect.

**Checkpointing** — Sauvegarde périodique de l'état d'un système pour pouvoir reprendre après une panne. Chandy-Lamport fournit un mécanisme de checkpoint cohérent.

**Consistent cut (coupure cohérente)** — Ensemble d'événements (un par processus) tel que : si une réception est dans le cut, l'envoi correspondant y est aussi. Le snapshot Chandy-Lamport produit toujours un consistent cut.

**Cut (coupure)** — Partition des événements d'un système distribué en « passé » et « futur » pour chaque processus. Un cut n'est pas forcément cohérent.

**Détection d'interblocage (Deadlock detection)** — Vérification qu'aucun cycle d'attente mutuelle n'existe entre processus. Un snapshot cohérent permet de le détecter sans arrêter le système.

**Détection de terminaison** — Vérifier qu'un calcul distribué est terminé : tous les processus sont inactifs et aucun message n'est en transit. Nécessite un snapshot complet.

**État global (Global state)** — Union de l'état local de chaque processus et de l'état de chaque canal (messages en transit).

**État local** — Variables, mémoire, compteurs d'un processus à un instant donné.

**Graphe fortement connexe** — Graphe orienté où tout sommet est accessible depuis tout autre sommet. Nécessaire pour que les marqueurs atteignent tous les processus.

**Horloge logique** — Mécanisme permettant d'ordonner les événements dans un système distribué sans horloge physique partagée (Lamport, 1978).

**Lai-Yang** — Algorithme de snapshot (1987) qui ne nécessite pas de canaux FIFO. Utilise un coloriage des messages (pré/post-snapshot) pour distinguer les époques.

**Lamport (Leslie)** — Informaticien, prix Turing 2013. Co-auteur de l'algorithme Chandy-Lamport. Auteur des horloges logiques, de l'algorithme de la boulangerie, et de Paxos.

**Marqueur (Marker)** — Message de contrôle envoyé par l'algorithme. Sert de frontière temporelle sur un canal : sépare les messages « avant le snapshot » de ceux « après ».

**Message en transit** — Message envoyé par un processus mais pas encore reçu par le destinataire. Fait partie de l'état du canal.

**Propriété stable** — Propriété qui, une fois vraie, reste vraie (ex : interblocage, terminaison). Un snapshot cohérent suffit pour la détecter, même s'il ne correspond pas à un état instantané réel.

**Snapshot (instantané)** — Capture de l'état global d'un système distribué à un moment logique. Comprend les états locaux et les messages en transit.

---

## Questions possibles et réponses

### Questions sur l'algorithme

**Q : Pourquoi le canal par lequel arrive le premier marqueur est-il noté « vide » ?**
- Parce que le canal est FIFO
- Tous les messages envoyés avant le marqueur ont déjà été reçus (ils sont arrivés avant le marqueur)
- Les messages envoyés après le marqueur sont « post-snapshot » → pas dans le snapshot
- Donc au moment du snapshot, rien n'est en transit sur ce canal

**Q : Pourquoi faut-il enregistrer les messages sur les autres canaux ?**
- Le processus a enregistré son état, mais les marqueurs des autres canaux n'ont pas encore atteint ce processus
- Des messages « pré-snapshot » peuvent encore arriver sur ces canaux
- Ces messages sont en transit au moment du snapshot → il faut les compter
- On arrête d'enregistrer quand le marqueur arrive sur chaque canal

**Q : L'algorithme bloque-t-il les processus ?**
- Non, les processus continuent de fonctionner normalement
- Ils envoient et reçoivent des messages applicatifs pendant le snapshot
- La seule contrainte : envoyer le marqueur avant tout autre message après l'enregistrement de l'état

**Q : Qui peut initier le snapshot ?**
- N'importe quel processus
- Plusieurs processus peuvent initier simultanément (mais en pratique on utilise un seul initiateur)
- Le premier marqueur reçu par un processus déclenche sa participation

**Q : Combien de messages l'algorithme envoie-t-il ?**
- Un marqueur par canal = O(E) messages, où E est le nombre d'arêtes
- Sur un graphe complet de N processus : N × (N-1) marqueurs
- C'est léger — aucun message applicatif n'est bloqué

---

### Questions sur les hypothèses

**Q : Que se passe-t-il si les canaux ne sont pas FIFO ?**
- Un message envoyé avant le marqueur pourrait arriver après
- Ce message ne serait pas enregistré → état incohérent (message « disparu »)
- Solutions : numéros de séquence pour recréer l'ordre, ou algorithmes alternatifs (Lai-Yang)

**Q : Que se passe-t-il si un marqueur est perdu ?**
- Le processus destinataire n'enregistrera jamais son état (ou n'arrêtera jamais d'enregistrer sur ce canal)
- Le snapshot ne se termine pas
- Hypothèse de canaux fiables nécessaire

**Q : Pourquoi le graphe doit-il être fortement connexe ?**
- Pour que les marqueurs puissent atteindre tous les processus
- Si un processus est inatteignable, on ne peut pas capturer son état
- En pratique, on peut relâcher à « tout processus est atteignable depuis l'initiateur »

**Q : TCP garantit-il FIFO ?**
- Oui, TCP garantit la livraison ordonnée et fiable
- Chandy-Lamport fonctionne directement sur TCP
- Sur UDP, il faudrait ajouter une couche de séquençage

---

### Questions sur la cohérence

**Q : Le snapshot correspond-il à un état qui a vraiment existé ?**
- Pas forcément — les processus enregistrent leur état à des moments différents
- Mais le snapshot est un **état atteignable** : le système aurait pu passer par cet état
- C'est un consistent cut : pas de réception sans envoi correspondant
- Suffisant pour détecter les propriétés stables

**Q : Qu'est-ce qu'une propriété stable ?**
- Une propriété qui, une fois vraie, reste vraie pour toujours
- Exemples : interblocage (un cycle d'attente ne se résout pas seul), terminaison (une fois fini, c'est fini)
- Contre-exemple : « le processus P a exactement 500€ » n'est pas stable
- Si le snapshot détecte une propriété stable, elle est effectivement vraie dans le système réel

**Q : Quelle différence entre cut et consistent cut ?**
- Un **cut** : un point d'observation par processus, quelconque
- Un **consistent cut** : si on observe la réception d'un message, on observe aussi son envoi
- Chandy-Lamport garantit un consistent cut grâce à FIFO + marqueurs

---

### Questions sur les applications

**Q : Comment Chandy-Lamport est-il utilisé dans Apache Flink ?**
- Flink fait du traitement de flux de données distribués
- Il utilise une variante appelée ABS (Asynchronous Barrier Snapshotting)
- Les « barriers » jouent le rôle des marqueurs
- Permet de faire du checkpointing sans arrêter le traitement
- En cas de panne, Flink reprend depuis le dernier snapshot cohérent

**Q : Comment détecter un interblocage avec un snapshot ?**
- On capture l'état global (qui attend quoi)
- On construit le graphe d'attente (wait-for graph)
- Si le graphe contient un cycle → interblocage détecté
- Propriété stable → si le snapshot le détecte, c'est vrai

**Q : Comment détecter la terminaison avec un snapshot ?**
- On capture l'état de chaque processus (actif/inactif) et l'état des canaux
- Si tous les processus sont inactifs ET tous les canaux sont vides → terminé
- Propriété stable → détection correcte

**Q : Peut-on utiliser Chandy-Lamport pour le débogage ?**
- Oui, on capture l'état du système sans l'arrêter
- On peut inspecter les variables de chaque processus et les messages en vol
- Limité : le snapshot est un point dans le temps, pas une trace continue

---

### Questions avancées

**Q : Peut-on faire plusieurs snapshots simultanément ?**
- Oui, en ajoutant un identifiant unique à chaque snapshot
- Chaque processus maintient un enregistrement par snapshot en cours
- Les marqueurs portent l'identifiant du snapshot auquel ils appartiennent

**Q : Quelle est la différence entre Chandy-Lamport et un backup classique ?**
- Un backup copie l'état d'une seule machine
- Chandy-Lamport capture l'état **distribué** : plusieurs machines + messages en transit
- Le problème est la cohérence entre les états, pas la copie elle-même

**Q : Pourquoi ne pas simplement arrêter tous les processus pour faire le snapshot ?**
- Arrêter tous les processus simultanément est aussi difficile que le problème initial (pas d'horloge globale)
- Même si on y arrivait, ça bloquerait le système → coûteux en performance
- Chandy-Lamport capture un état cohérent sans interruption

**Q : Quel lien entre les horloges de Lamport et cet algorithme ?**
- Les horloges de Lamport (1978) donnent un ordre partiel sur les événements
- Chandy-Lamport (1985) capture un état global cohérent
- Les deux résolvent des problèmes liés à l'absence d'horloge globale
- Les marqueurs jouent un rôle similaire aux timestamps : ils créent une frontière logique
