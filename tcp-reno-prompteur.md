# TCP Reno — Script prompteur (10 min)



---

## [0:00 — 1:00] Introduction

Bonjour. // Je vais vous présenter **TCP Reno**, // un algorithme de contrôle de congestion publié en 1990 // et qui reste aujourd'hui la référence pédagogique dès qu'on parle de TCP. //

En octobre 1986, // le réseau NSFNET — l'ancêtre d'Internet — // subit son premier effondrement par congestion. // Sur une liaison entre deux laboratoires distants de 400 mètres, // le débit utile passe de **32 kilobits par seconde à 40 bits par seconde**, // soit un facteur mille. //

Le mécanisme est simple : // les émetteurs TCP retransmettent les paquets perdus, // ce qui sature davantage les routeurs, // qui perdent davantage de paquets, // qui déclenchent davantage de retransmissions. // C'est un effet d'avalanche. //

En 1988, Van Jacobson publie un article fondateur // qui propose les premiers mécanismes pour éviter ce phénomène. // La première implémentation s'appelle **TCP Tahoe**. // Deux ans plus tard, // une version améliorée est publiée : **TCP Reno**. // C'est celle-ci qu'on va détailler.

---

## [1:00 — 2:00] Notions préalables

Trois termes de vocabulaire avant d'entrer dans l'algorithme. //

TCP fonctionne avec une **fenêtre glissante**. // À tout instant, // l'émetteur peut avoir un certain nombre d'octets en vol, // c'est-à-dire envoyés mais pas encore acquittés. // Cette quantité maximale s'appelle la **fenêtre de congestion**, // notée **`cwnd`**. // C'est la variable centrale de Reno. // Tout l'algorithme consiste à faire varier `cwnd` dans le temps. //

Deuxième terme : **`ssthresh`**, // pour *slow start threshold*. // C'est un seuil qui sépare deux régimes de croissance de `cwnd`. //

Troisième point : Reno reconnaît **deux signaux de congestion distincts**. // Le premier est le **timeout** : // l'émetteur attend un acquittement, il n'arrive jamais, le timer expire. // C'est un signal fort. // Le second est la réception de **trois acquittements dupliqués** : // le récepteur renvoie trois fois de suite le même ACK. // C'est un signal faible — // un paquet manque, mais les autres continuent d'arriver. //

Cette distinction entre signal fort et signal faible // est au cœur de ce qui fait la différence entre Reno et son prédécesseur Tahoe.

---

## [2:00 — 6:00] Les quatre mécanismes de Reno

Reno combine quatre algorithmes. // Je les présente dans l'ordre.

### Mécanisme 1 — Slow Start

Le premier est le **Slow Start**. //

Au début d'une connexion, // l'émetteur n'a aucune information sur le chemin réseau. // Il ne connaît ni sa capacité, ni le nombre de flux concurrents. // Il commence donc avec `cwnd` égal à **un seul segment**. //

À chaque acquittement reçu, // il **ajoute un segment à `cwnd`**. // Concrètement : // au départ `cwnd` vaut 1. // Un ACK arrive, `cwnd` passe à 2. // Deux ACK arrivent, `cwnd` passe à 4. // Puis 8, puis 16. // **`cwnd` double à chaque RTT.** //

Malgré son nom, // Slow Start produit donc une croissance **exponentielle**. // Le mot « slow » fait référence au point de départ bas, // pas au rythme de croissance. //

Cette croissance continue tant que `cwnd` reste inférieure à `ssthresh`. // Dès que `cwnd` atteint ce seuil, // on bascule dans le deuxième mécanisme.

### Mécanisme 2 — Congestion Avoidance

Le deuxième mécanisme est la **Congestion Avoidance**, // l'évitement de congestion. //

Quand `cwnd` atteint `ssthresh`, // on estime qu'on approche de la capacité du lien. // Doubler la fenêtre deviendrait dangereux. // Reno passe alors en croissance linéaire. //

La règle : // `cwnd` augmente d'**un seul segment par RTT**, // au lieu de doubler. // On sonde la capacité disponible segment par segment, // jusqu'à ce qu'une perte se produise. //

Ce comportement définit **AIMD**, // pour *Additive Increase, Multiplicative Decrease* : // **augmentation additive** quand tout va bien, // **diminution multiplicative** quand une perte survient. // AIMD est au cœur de TCP, // et c'est ce qui produit la **courbe en dents de scie** // caractéristique de Reno.

### Mécanisme 3 — Fast Retransmit

Le troisième mécanisme est le **Fast Retransmit**, // la retransmission rapide. //

Quand trois ACK dupliqués arrivent, // un segment manque mais les suivants ont bien été reçus. //

Avant Reno, // l'émetteur attendait l'expiration du timer **RTO** pour retransmettre. // Un RTO peut durer plusieurs centaines de millisecondes, // pendant lesquelles la connexion est bloquée sans raison. //

Reno retransmet **dès le troisième ACK dupliqué**, // sans attendre le timer. // Le seuil de trois est un compromis empirique : // un ou deux ACK dupliqués peuvent résulter d'un simple **réordonnancement** des paquets dans le réseau, // sans perte réelle. // Trois est considéré comme suffisamment fiable // pour distinguer une vraie perte d'un réordonnancement.

### Mécanisme 4 — Fast Recovery

Le quatrième mécanisme est le **Fast Recovery**. // C'est la contribution principale de Reno par rapport à Tahoe. //

Le comportement de Tahoe, d'abord. // Sur trois ACK dupliqués, // Tahoe remet `cwnd` à 1 // et repart en Slow Start depuis le début. // Le pipe se vide entièrement // et plusieurs RTT sont nécessaires pour le remplir à nouveau. //

Reno adopte un raisonnement différent. // Des ACK dupliqués signifient que **des paquets continuent d'arriver** au récepteur. // La connexion fonctionne encore. // Tomber à `cwnd = 1` serait excessif. //

La procédure de Fast Recovery est la suivante. // À la réception du troisième ACK dupliqué : // `ssthresh` est fixé à la **moitié** de `cwnd`, // puis `cwnd` est fixé à `ssthresh + 3 segments`, // et le segment manquant est retransmis. // À la réception du nouvel ACK qui acquitte la retransmission, // `cwnd` est fixé à `ssthresh` // et on **reprend directement en Congestion Avoidance**, // sans repasser par Slow Start. //

Le résultat : // `cwnd` est **divisée par deux** au lieu de tomber à 1. // Le pipe reste globalement rempli. // L'efficacité est bien meilleure sur les pertes isolées. //

Une précision importante : // si la perte est détectée par un **timeout**, // Reno reste prudent et se comporte comme Tahoe. // `cwnd` à 1, retour en Slow Start. // Un timeout reste un signal grave, // qui indique que la connexion est probablement fortement dégradée.

---

## [6:00 — 7:00] La courbe en dents de scie

L'enchaînement de ces quatre mécanismes produit la **courbe en dents de scie** caractéristique de Reno. //

Au début, // une montée raide correspondant au Slow Start exponentiel. // Puis une pente plus douce, // la Congestion Avoidance linéaire. // Quand une perte est détectée par trois ACK dupliqués, // `cwnd` est divisée par deux — // c'est une chute verticale. // Puis la croissance linéaire reprend. // Nouvelle perte, nouvelle chute de moitié, // et ainsi de suite. //

Cette signature en dents de scie // est l'image qu'il faut associer à Reno. // C'est aussi ce qui la distingue visuellement de Tahoe, // dont `cwnd` retombe à zéro à chaque perte // au lieu d'être divisée par deux.

---

## [7:00 — 8:30] Les limites de Reno

Reno présente un défaut connu : // il gère mal les **pertes multiples dans une même fenêtre**. //

Le problème vient du fait que Fast Recovery ne récupère **qu'une seule perte par RTT**. // Si deux segments sont perdus dans la même fenêtre, // Reno sort de Fast Recovery dès que le premier nouvel ACK arrive, // en considérant que tout est résolu. // Le second segment perdu déclenche alors soit un nouveau Fast Recovery, // soit, dans le pire cas, un timeout. // Si on tombe en timeout, // `cwnd` redescend à 1 // et tous les bénéfices de Fast Recovery sont perdus. //

Ce défaut a motivé plusieurs améliorations successives. //

**NewReno**, publié en 1999, // reste en Fast Recovery tant que **tous** les segments perdus n'ont pas été acquittés. //

**TCP SACK**, pour *Selective Acknowledgment*, // permet au récepteur d'indiquer **précisément** quels segments lui manquent. // L'émetteur n'a plus besoin de deviner. //

**CUBIC**, // qui utilise une fonction cubique pour la croissance de `cwnd`, // est optimisé pour les réseaux haut débit à grande latence. // C'est l'algorithme par défaut de Linux depuis 2006. //

**BBR**, développé par Google en 2016, // modélise directement la bande passante et le RTT du chemin, // sans utiliser les pertes comme signal principal de congestion.

---

## [8:30 — 10:00] Conclusion

Reno est techniquement dépassé, // mais il reste enseigné pour trois raisons. //

**Première raison** : // c'est l'archétype du contrôle de congestion. // Les quatre mécanismes présentés — // Slow Start, Congestion Avoidance, Fast Retransmit, Fast Recovery — // structurent encore tous les algorithmes TCP modernes. // CUBIC, NewReno, et dans une moindre mesure BBR, // héritent directement de cette architecture. //

**Deuxième raison** : // il existe un critère appelé **Reno-friendliness**. // Tout nouvel algorithme de contrôle de congestion doit prouver qu'il **partage équitablement** la bande passante avec un flux Reno. // Sans cette propriété, // il est considéré comme agressif et n'est pas accepté dans les RFC. // Reno reste donc le mètre étalon de l'équité entre flux TCP. //

**Troisième raison** : // Reno est assez simple pour être analysé mathématiquement. // La **formule de Mathis** donne le débit moyen d'un flux Reno // en fonction du MSS, du RTT et du taux de pertes. // Ce type de modèle permet de prédire le comportement de TCP // sans passer par la simulation. //

En résumé : // Reno n'est plus utilisé en production, // mais il reste la référence intellectuelle et le test de compatibilité // pour tout ce qui touche au contrôle de congestion dans TCP. //

Je vous remercie pour votre attention. // Je suis prêt à répondre à vos questions.

---

## Notes pour la répétition

- **Mots à articuler clairement** : `cwnd`, `ssthresh`, AIMD, Reno-friendliness
- **Temps moyen total** : ~10 min à 130 mots/min
- **Si tu débordes** : couper la précision sur le timeout à la fin de Fast Recovery (~15 s) et réduire la section limites à NewReno + SACK uniquement (~20 s)
- **Si tu finis trop tôt** : ralentir sur Fast Recovery, qui est le point central et mérite d'être posé
