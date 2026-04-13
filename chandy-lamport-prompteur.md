# Chandy-Lamport — Script prompteur (10 min)



---

## [0:00 — 1:30] Introduction et problème

Bonjour. // Je vais vous présenter l'algorithme de **Chandy-Lamport**, // publié en 1985, // qui permet de capturer l'état global d'un système distribué // sans arrêter le système. //

Dans un système distribué, // chaque processus a sa propre mémoire, // sa propre horloge, // et communique avec les autres par **envoi de messages**. // Il n'existe pas d'horloge globale. // Il n'existe pas de mémoire partagée. //

Le problème est le suivant. // On veut connaître l'état complet du système à un moment donné — // l'état de chaque processus, // plus les messages qui sont en transit dans les canaux. // Mais on ne peut pas demander à tout le monde de s'arrêter au même instant, // parce qu'il n'y a **pas d'instant commun**. //

Ce besoin est concret. // On utilise un snapshot global pour la **détection d'interblocage**, // pour la **détection de terminaison** d'un calcul distribué, // ou encore pour le **checkpointing** — // c'est-à-dire sauvegarder un état cohérent du système // afin de pouvoir y revenir après une panne. //

L'algorithme de Chandy-Lamport résout ce problème // de manière élégante et peu coûteuse.

---

## [1:30 — 3:00] Modèle et hypothèses

Avant de décrire l'algorithme, // il faut poser le modèle. // Chandy-Lamport repose sur des hypothèses strictes. //

Première hypothèse : // les canaux de communication sont **FIFO**. // Les messages arrivent dans l'ordre dans lequel ils ont été envoyés. // C'est l'hypothèse la plus importante — // sans elle, l'algorithme ne fonctionne pas. //

Deuxième hypothèse : // les canaux sont **fiables**. // Pas de perte, pas de duplication, pas de corruption de message. //

Troisième hypothèse : // le graphe de communication est **fortement connexe**. // Tout processus peut atteindre tout autre processus, // directement ou par transitivité. //

Avec ces hypothèses en tête, // on peut définir ce qu'on cherche à capturer. // L'**état global** du système est composé de deux choses : // l'**état local** de chaque processus — // ses variables, ses compteurs, sa mémoire — // et l'**état de chaque canal**, // c'est-à-dire les messages en transit, // envoyés mais pas encore reçus. // Le snapshot doit capturer les deux.

---

## [3:00 — 5:30] L'algorithme

L'outil central de l'algorithme est un message spécial appelé **marqueur**. // Ce n'est pas un message applicatif. // C'est un message de contrôle // dont le seul rôle est de servir de **frontière** // entre les messages « avant le snapshot » et « après le snapshot ». //

L'algorithme fonctionne en deux temps. //

**Premier temps — l'initiation.** // Un processus, appelons-le P1, // décide de lancer le snapshot. // P1 **enregistre son propre état local**, // puis envoie un **marqueur** sur chacun de ses canaux sortants. // Il ne doit envoyer aucun autre message avant les marqueurs. //

**Deuxième temps — la réception d'un marqueur.** // Quand un processus P reçoit un marqueur sur un canal C, // deux cas se présentent. //

**Cas 1** : c'est le **premier marqueur** que P reçoit. // P n'a pas encore enregistré son état. // Alors P **enregistre son état local** immédiatement. // Le canal C par lequel le marqueur est arrivé est noté comme **vide** — // puisque le canal est FIFO, // tous les messages envoyés avant le marqueur ont déjà été reçus. // P commence ensuite à **enregistrer tous les messages** // qui arrivent sur ses **autres canaux entrants**. // Et P envoie un marqueur sur chacun de ses canaux sortants. //

**Cas 2** : P a **déjà** enregistré son état, // parce qu'il avait déjà reçu un marqueur d'un autre processus. // Alors P **arrête d'enregistrer** les messages sur le canal C. // Les messages qu'il a enregistrés sur C depuis l'enregistrement de son état // constituent l'**état du canal C** dans le snapshot. // Ce sont les messages qui étaient en transit au moment du snapshot. //

L'algorithme se termine quand chaque processus // a reçu un marqueur sur chacun de ses canaux entrants. // À ce moment, on a l'état local de chaque processus // et l'état de chaque canal. // C'est le snapshot global.

---

## [5:30 — 8:00] Exemple concret

On va dérouler l'algorithme sur un exemple. // Trois processus : **P1**, **P2**, **P3**, // reliés en triangle — chacun peut envoyer à chacun. //

Au départ, // P1 a un solde de **500 euros**, // P2 a **300 euros**, // P3 a **200 euros**. // Total : **1000 euros**. // C'est un invariant — il ne doit pas y avoir de création ni de disparition d'argent. //

Le déroulement est le suivant. //

P1 envoie **100 euros à P2**. // Ce message, appelons-le M1, // est en transit dans le canal P1→P2. // P1 passe à **400 euros**. //

Juste après, // P1 décide d'initier le snapshot. // P1 enregistre son état : **400 euros**. // P1 envoie un marqueur à P2 et à P3. //

Côté P2. // P2 reçoit d'abord M1 — // les 100 euros — // avant le marqueur, // parce que M1 a été envoyé avant le marqueur et que le canal est FIFO. // P2 passe à **400 euros**. // Puis P2 reçoit le marqueur de P1. // C'est son premier marqueur. // Il enregistre son état : **400 euros**. // Le canal P1→P2 est noté **vide**. // P2 commence à enregistrer sur le canal P3→P2. // P2 envoie un marqueur à P1 et P3. //

Pendant ce temps, // P3 envoie **50 euros à P2** — // message M2, en transit sur P3→P2. // P3 passe à **150 euros**. //

P3 reçoit le marqueur de P1. // Premier marqueur pour P3. // P3 enregistre son état : **150 euros**. // Canal P1→P3 : **vide**. // P3 envoie un marqueur à P1 et P2. //

Retour côté P2. // P2 reçoit M2 — // les 50 euros de P3. // Ce message est arrivé **avant** le marqueur de P3 // sur le canal P3→P2. // P2 l'enregistre. // Puis P2 reçoit le marqueur de P3. // P2 arrête d'enregistrer sur P3→P2. // L'état du canal P3→P2 est : **M2, soit 50 euros en transit**. //

Les autres marqueurs arrivent : // P1 reçoit ceux de P2 et P3, // P3 reçoit celui de P2. // Tous les canaux restants sont vides. //

**Résultat du snapshot** : // P1 : 400 euros. // P2 : 400 euros. // P3 : 150 euros. // Canal P3→P2 : 50 euros en transit. // Total : 400 + 400 + 150 + 50 = **1000 euros**. // L'invariant est respecté. // Rien n'a été perdu, rien n'a été dupliqué.

---

## [8:00 — 9:00] Pourquoi ça marche et propriétés

L'algorithme fonctionne grâce à l'hypothèse FIFO. // Le marqueur arrive sur un canal **après** tous les messages envoyés avant lui // et **avant** tous les messages envoyés après. // Il agit comme un **séparateur** temporel. //

Le snapshot produit est ce qu'on appelle un **cut cohérent** : // si un message est comptabilisé comme reçu dans l'état d'un processus, // alors il est aussi comptabilisé comme envoyé — // il n'apparaît pas dans l'état de l'émetteur. // Si un message est en transit dans un canal, // il a bien été soustrait de l'émetteur mais pas ajouté au récepteur. // Pas de duplication, pas de perte. //

Remarque importante : // l'état global capturé par le snapshot // n'a pas forcément existé à un instant précis dans le temps. // Mais il est **atteignable** — // c'est un état par lequel le système aurait pu passer. // Pour détecter des propriétés stables // comme l'interblocage ou la terminaison, // c'est suffisant. //

En termes de coût, // l'algorithme envoie **un marqueur par canal**, // soit O(E) messages supplémentaires, // où E est le nombre d'arêtes du graphe. // Il ne bloque aucun processus.

---

## [9:00 — 10:00] Limites et conclusion

L'algorithme a des limites connues. // La première : il **exige** des canaux FIFO. // Sur un réseau où les messages peuvent se doubler — // par exemple avec UDP — // il faut recréer l'ordre avec des numéros de séquence. //

La deuxième : il ne **tolère pas les pannes**. // Si un processus crash pendant le snapshot, // le snapshot est incomplet. //

Ces limites ont motivé des extensions. // L'algorithme de **Lai-Yang** en 1987 // supprime l'hypothèse FIFO // en coloriant les messages. // Plus récemment, // **Apache Flink** utilise une variante de Chandy-Lamport // pour faire du checkpointing de flux de données distribués en temps réel. //

En résumé. // Chandy-Lamport résout un problème fondamental des systèmes distribués : // capturer un état global cohérent // sans horloge globale, sans mémoire partagée, // et sans arrêter le système. // L'outil est un message marqueur qui sert de frontière temporelle. // L'hypothèse clé est le FIFO. // Le résultat est un cut cohérent. // L'algorithme est léger — un marqueur par canal — // et il est à la base de tous les mécanismes modernes de checkpointing distribué. //

Je vous remercie pour votre attention. // Je suis prêt à répondre à vos questions.

---

## Notes pour la répétition

- **Mots à articuler clairement** : marqueur, FIFO, cut cohérent, checkpointing, interblocage
- **Temps moyen total** : ~10 min à 130 mots/min
- **Si tu débordes** : raccourcir l'exemple en ne détaillant que P1 et P2, mentionner P3 sans dérouler (~30 s gagnées). Couper la partie Lai-Yang/Flink (~15 s)
- **Si tu finis trop tôt** : ralentir sur l'exemple, poser les valeurs clairement, et insister sur le rôle du FIFO à chaque étape
- **Point d'appui** : l'exemple des 1000€ est le fil rouge — tout le monde comprend qu'on ne veut pas perdre d'argent
