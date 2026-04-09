# TCP Reno — Document complémentaire

---

## Mémo rapide

| Phase | Événement | Action sur `cwnd` | Régime suivant |
|---|---|---|---|
| **Slow Start** | ACK reçu | `cwnd += 1 segment` (doublement par RTT) | Continue jusqu'à `cwnd >= ssthresh` |
| **Congestion Avoidance** | ACK reçu | `cwnd += 1/cwnd` (~1 segment par RTT) | Continue jusqu'à perte |
| **Fast Retransmit** | 3 ACK dupliqués | Retransmission immédiate du segment manquant | Fast Recovery |
| **Fast Recovery** | 3 ACK dupliqués | `ssthresh = cwnd/2`, `cwnd = ssthresh + 3` | Congestion Avoidance (dès nouvel ACK) |
| **Timeout** | Timer expire | `ssthresh = cwnd/2`, `cwnd = 1` | Slow Start |

**Formule AIMD** : croissance additive (+1 segment/RTT), décroissance multiplicative (cwnd / 2).

**Courbe en dents de scie** : montée exponentielle (Slow Start) puis linéaire (Congestion Avoidance), chute de moitié (Fast Recovery) ou à 1 (timeout), et on recommence.

---

## Glossaire — Définitions des termes clés

**ACK (Acknowledgment)** — Message de confirmation envoyé par le récepteur pour indiquer qu'un segment a bien été reçu.

**ACK dupliqué** — ACK portant le même numéro de séquence qu'un ACK précédent. Indique qu'un segment attendu n'est pas arrivé, mais que des segments ultérieurs sont reçus.

**AIMD (Additive Increase, Multiplicative Decrease)** — Stratégie de contrôle : augmentation linéaire de `cwnd` en régime normal, division par deux en cas de perte. Garantit la convergence vers un partage équitable de la bande passante.

**BBR (Bottleneck Bandwidth and RTT)** — Algorithme de congestion de Google (2016) qui mesure directement la bande passante et le RTT au lieu d'utiliser les pertes comme signal.

**Congestion** — Surcharge d'un noeud du réseau (routeur) dont les files d'attente débordent, provoquant des pertes de paquets.

**Congestion Avoidance** — Phase de croissance linéaire de `cwnd` (+1 segment/RTT) une fois `ssthresh` atteint.

**CUBIC** — Algorithme de congestion par défaut de Linux depuis 2006. Utilise une fonction cubique pour la croissance de `cwnd`, optimisé pour les liens à fort BDP (bandwidth-delay product).

**cwnd (Congestion Window)** — Nombre maximal d'octets que l'émetteur peut avoir en vol (envoyés mais non acquittés). Variable centrale de Reno.

**Effondrement par congestion (Congestion collapse)** — Situation où le réseau est saturé de retransmissions inutiles, réduisant le débit utile à quasi-zéro. Événement fondateur : NSFNET, octobre 1986.

**Fast Recovery** — Après 3 ACK dupliqués, `cwnd` est divisé par deux (au lieu de tomber à 1). On reprend directement en Congestion Avoidance. Innovation principale de Reno par rapport à Tahoe.

**Fast Retransmit** — Retransmission immédiate du segment manquant dès réception de 3 ACK dupliqués, sans attendre le timeout.

**Fenêtre glissante (Sliding Window)** — Mécanisme TCP permettant d'envoyer plusieurs segments sans attendre l'acquittement de chacun. La taille de la fenêtre limite le nombre d'octets en vol.

**Formule de Mathis** — `Débit ≈ (MSS / RTT) × (C / √p)` où `p` est le taux de perte et `C` une constante (~1.22). Modèle analytique du débit d'un flux Reno.

**MSS (Maximum Segment Size)** — Taille maximale des données dans un segment TCP (typiquement 1460 octets sur Ethernet).

**NewReno** — Amélioration de Reno (RFC 3782, 1999) qui reste en Fast Recovery tant que tous les segments perdus ne sont pas acquittés.

**NSFNET** — Réseau académique américain, ancêtre d'Internet. Lieu de l'effondrement par congestion de 1986.

**Pipe** — Ensemble des segments actuellement en transit dans le réseau (envoyés, pas encore acquittés).

**Reno-friendliness** — Critère imposant qu'un nouvel algorithme de congestion partage équitablement la bande passante avec un flux Reno coexistant.

**RTT (Round-Trip Time)** — Temps d'aller-retour d'un paquet entre l'émetteur et le récepteur. Détermine la vitesse à laquelle `cwnd` évolue.

**RTO (Retransmission Timeout)** — Délai après lequel l'émetteur considère qu'un segment est perdu s'il n'a pas reçu d'ACK. Calculé dynamiquement à partir du RTT.

**SACK (Selective Acknowledgment)** — Extension TCP permettant au récepteur d'indiquer précisément les plages de segments reçus, ce qui permet à l'émetteur de ne retransmettre que les segments manquants.

**Segment** — Unité de données TCP. Contient un en-tête TCP et une charge utile (payload) de taille ≤ MSS.

**Slow Start** — Phase initiale de TCP où `cwnd` double à chaque RTT (croissance exponentielle). Nom trompeur : la croissance est rapide, c'est le point de départ qui est bas.

**ssthresh (Slow Start Threshold)** — Seuil séparant Slow Start (exponentiel) et Congestion Avoidance (linéaire). Mis à jour à `cwnd/2` à chaque événement de perte.

**Timeout** — Expiration du timer RTO. Signal de congestion grave. Provoque `cwnd = 1` et retour en Slow Start.

**Van Jacobson** — Chercheur qui a publié en 1988 les algorithmes fondateurs du contrôle de congestion TCP (papier : "Congestion Avoidance and Control").

---

## Questions possibles et réponses

### Questions sur les mécanismes

**Q : Pourquoi le seuil est-il fixé à 3 ACK dupliqués et pas 2 ou 4 ?**
C'est un compromis empirique. Un ou deux ACK dupliqués peuvent résulter d'un simple réordonnancement des paquets dans le réseau, pas d'une perte. Trois réduit les faux positifs tout en restant réactif. Ce seuil a été validé expérimentalement et n'a pas de justification mathématique stricte.

**Q : Pourquoi `cwnd = ssthresh + 3` et pas juste `ssthresh` en Fast Recovery ?**
Les 3 segments ajoutés correspondent aux 3 ACK dupliqués déjà reçus. Chaque ACK dupliqué prouve qu'un segment est sorti du réseau (reçu par le destinataire). On peut donc en envoyer un de plus. C'est une forme de "window inflation" : on maintient le pipe rempli pendant la récupération.

**Q : Que se passe-t-il si un ACK dupliqué arrive après le troisième pendant Fast Recovery ?**
Chaque ACK dupliqué supplémentaire incrémente `cwnd` de 1 segment. Cela permet de continuer à émettre de nouveaux segments pendant que le segment perdu est en cours de retransmission. Dès qu'un nouvel ACK (non dupliqué) arrive, `cwnd` retombe à `ssthresh`.

**Q : Pourquoi ne pas toujours utiliser Fast Recovery, même sur un timeout ?**
Un timeout signifie qu'aucun ACK n'arrive du tout, même pas des ACK dupliqués. Le réseau est probablement très congestionné ou le chemin est coupé. Rester à un `cwnd` élevé aggraverait la situation. Revenir à 1 est la réponse la plus conservatrice et la plus sûre.

**Q : Comment `ssthresh` est-il initialisé au tout début de la connexion ?**
À une valeur très grande (typiquement 65535 octets ou l'infini logique). Cela garantit que la première phase est entièrement en Slow Start. `ssthresh` ne prend une valeur significative qu'après la première perte détectée.

**Q : Slow Start est-il vraiment "slow" ?**
Non. La croissance est exponentielle (doublement par RTT). Le nom fait référence au fait que l'émetteur démarre à `cwnd = 1` au lieu d'envoyer immédiatement à pleine capacité. Par rapport à l'absence de contrôle de congestion, c'est un démarrage lent. Par rapport à Congestion Avoidance, c'est en réalité très rapide.

---

### Questions sur les performances et comparaisons

**Q : Quelle est la différence principale entre Tahoe et Reno ?**
Tahoe remet toujours `cwnd` à 1 en cas de perte (que ce soit timeout ou 3 ACK dupliqués). Reno distingue les deux cas : sur 3 ACK dupliqués, il divise `cwnd` par 2 (Fast Recovery) au lieu de tomber à 1. Sur timeout, les deux se comportent de la même manière.

**Q : Quelle est la différence entre Reno et NewReno ?**
Reno sort de Fast Recovery dès le premier nouvel ACK. Si plusieurs segments étaient perdus dans la même fenêtre, les pertes suivantes déclenchent un nouveau cycle (voire un timeout). NewReno reste en Fast Recovery et retransmet les segments manquants un par un, tant que tous n'ont pas été acquittés. Résultat : NewReno gère bien les pertes multiples.

**Q : Quelle est la différence entre Reno et CUBIC ?**
Reno utilise une croissance linéaire (+1 segment/RTT) en Congestion Avoidance. CUBIC utilise une fonction cubique centrée sur le dernier `cwnd` avant perte. CUBIC est plus agressif pour remonter vers la capacité sur les liens à fort BDP (haut débit, grande latence), ce qui le rend mieux adapté aux réseaux modernes.

**Q : Pourquoi CUBIC a remplacé Reno en pratique ?**
Les réseaux modernes ont des BDP (bandwidth-delay product) beaucoup plus grands qu'en 1990. Avec Reno, remonter linéairement segment par segment après une perte peut prendre des dizaines de secondes sur un lien à 10 Gbps. CUBIC remonte beaucoup plus vite grâce à sa fonction cubique.

**Q : Pourquoi BBR est-il fondamentalement différent ?**
Les algorithmes classiques (Reno, CUBIC) utilisent la perte de paquets comme signal de congestion. BBR mesure directement la bande passante disponible et le RTT minimum. Il ajuste son débit pour remplir le lien sans remplir les buffers. Cela le rend plus efficace sur les réseaux avec de gros buffers (bufferbloat).

**Q : TCP Reno est-il équitable entre flux ?**
Oui, sous certaines conditions. Si deux flux Reno partagent le même lien avec le même RTT, AIMD les fait converger vers un partage 50/50 de la bande passante. En revanche, un flux avec un RTT plus court augmente son `cwnd` plus vite et obtient une part plus grande. C'est le problème du RTT bias.

---

### Questions sur le contexte et l'histoire

**Q : Qui est Van Jacobson ?**
Chercheur américain, alors au Lawrence Berkeley National Laboratory. Son papier de 1988 "Congestion Avoidance and Control" a introduit Slow Start et Congestion Avoidance. Il est aussi co-auteur de `traceroute` et a contribué à la compression d'en-têtes TCP/IP (Van Jacobson compression).

**Q : Qu'est-ce que l'effondrement de NSFNET en 1986 ?**
Le réseau académique NSFNET a vu son débit utile chuter d'un facteur 1000 (de 32 kbps à 40 bps) sur un lien de 400 mètres. Cause : les émetteurs TCP retransmettaient les paquets perdus, ce qui saturait davantage les routeurs, qui perdaient davantage de paquets. Boucle de rétroaction positive (effet d'avalanche).

**Q : Dans quelle RFC est défini TCP Reno ?**
Reno n'a pas de RFC unique. Les mécanismes sont décrits dans la RFC 5681 (TCP Congestion Control, 2009) qui consolide les RFC 2001 et 2581. Fast Recovery est détaillé dans la RFC 2001 (1997, Stevens). Le terme "Reno" vient du noyau BSD 4.3 Reno (1990).

**Q : Reno est-il encore utilisé quelque part en production ?**
Très rarement de manière intentionnelle. Linux utilise CUBIC par défaut depuis 2006, Windows utilise CUBIC depuis Windows 10, macOS utilise CUBIC également. Certains systèmes embarqués ou IoT très anciens pourraient encore utiliser Reno, mais ce n'est plus le standard.

---

### Questions sur les applications et cas concrets

**Q : Que se passe-t-il concrètement quand on télécharge un fichier avec TCP Reno ?**
Au début, le débit monte très vite (Slow Start exponentiel). En quelques RTT, on atteint la capacité du lien. Une perte se produit, `cwnd` est divisé par deux, le débit chute. Puis il remonte linéairement. Le débit oscille en dents de scie autour de la capacité du lien.

**Q : Pourquoi TCP n'envoie-t-il pas simplement à la vitesse maximale du lien dès le début ?**
Parce que l'émetteur ne connaît pas la capacité du chemin de bout en bout (le goulot d'étranglement peut être n'importe où). Et même s'il la connaissait, d'autres flux partagent le même lien. Envoyer à pleine vitesse sans adaptation provoquerait de la congestion et des pertes massives, comme en 1986.

**Q : Quel lien entre contrôle de congestion et contrôle de flux ?**
Le contrôle de flux (via `rwnd`, receiver window) protège le récepteur contre un émetteur trop rapide. Le contrôle de congestion (via `cwnd`) protège le réseau contre trop de trafic. La fenêtre effective est `min(cwnd, rwnd)`. Les deux mécanismes sont indépendants mais coexistent.

**Q : Que se passe-t-il si le réseau n'est pas congestionné du tout ?**
`cwnd` croît en Slow Start jusqu'à `ssthresh` (initialement très grand), puis en Congestion Avoidance. Si aucune perte ne survient, `cwnd` continue de croître indéfiniment, limité seulement par `rwnd` (la fenêtre du récepteur). En pratique, une perte finit toujours par arriver.

**Q : La formule de Mathis, c'est quoi exactement ?**
`Débit ≈ (MSS / RTT) × (C / √p)` avec `p` le taux de perte et `C ≈ 1.22`. Elle montre que le débit TCP est inversement proportionnel au RTT et à la racine carrée du taux de perte. Conséquence : doubler le RTT divise le débit par deux. Diviser le taux de perte par quatre double le débit.

---

### Questions pièges / avancées

**Q : AIMD garantit-il l'équité ? Pourquoi ?**
Oui, sous hypothèse de RTT identiques. On peut le montrer graphiquement : dans un diagramme à deux flux, AIMD converge vers la diagonale (partage égal) car l'augmentation additive déplace les deux flux parallèlement à la bissectrice, tandis que la diminution multiplicative les rapproche de l'origine en ligne droite. La combinaison des deux fait converger vers l'intersection de la droite d'équité et de la droite de capacité.

**Q : Pourquoi diviser par 2 et pas par 3 ou par 4 ?**
Le facteur 1/2 est un compromis entre réactivité et efficacité. Diviser par plus (ex : par 4) libérerait trop de bande passante et serait trop conservateur. Diviser par moins (ex : par 1.5) risquerait de ne pas réduire assez pour soulager la congestion. Le facteur 1/2 est aussi celui qui donne la convergence AIMD la plus propre mathématiquement.

**Q : TCP peut-il fonctionner sans contrôle de congestion ?**
Techniquement oui, mais c'est dangereux pour le réseau. Sans contrôle de congestion, un émetteur peut saturer le réseau et provoquer un effondrement comme en 1986. C'est pour cela que le contrôle de congestion est obligatoire dans les RFC (RFC 5681). Un flux sans contrôle de congestion serait considéré comme "non coopératif".

**Q : Qu'est-ce que le bufferbloat et quel rapport avec Reno ?**
Le bufferbloat est l'augmentation excessive de la latence causée par des buffers trop grands dans les routeurs. Reno (et CUBIC) remplissent les buffers avant de détecter une perte, ce qui augmente le RTT. BBR tente de résoudre ce problème en ne remplissant pas les buffers.

**Q : Quelle est la différence entre `cwnd` et `rwnd` ?**
`cwnd` (congestion window) est contrôlée par l'émetteur et protège le réseau. `rwnd` (receiver window) est annoncée par le récepteur et protège le récepteur. La fenêtre effective d'émission est `min(cwnd, rwnd)`.
