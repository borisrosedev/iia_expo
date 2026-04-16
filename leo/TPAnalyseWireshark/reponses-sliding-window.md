# TP Informatique — Réponses

**Nom :** Bonnet
**Prénom :** Antoine 
**Date :**  8 avril  

---

## Partie 1 — Observation du sliding window dans Wireshark

> **Contexte :** Capture Wireshark lors du téléchargement d’un fichier.
> - Taille d’un segment TCP : 1460 octets
> - RTT moyen : 40 ms
> - Taille de la fenêtre TCP : 8760 octets

### Question 1.1

> _Combien de segments TCP peuvent être envoyés sans attendre d’acquittement ?_

**Réponse :** 8760 / 1460 = **6 segments** peuvent être envoyés sans attendre d’ACK.

---

### Question 1.2

> _Combien d’octets peuvent être envoyés sans attendre d’ACK ?_

**Réponse :** La taille de la fenêtre TCP correspond au nombre d’octets pouvant être en transit simultanément : **8760 octets**.

---

### Question 1.3

> _Combien d’acquittements sont nécessaires pour transmettre 43800 octets ?_

**Réponse :** 43800 / 8760 = **5 acquittements** sont nécessaires (un ACK par fenêtre complète reçue).

---

### Question 1.4

> _Quelle est la relation entre la taille de la fenêtre et le nombre de segments envoyés ?_

**Réponse :** Le nombre de segments envoyés sans attendre d’ACK est égal à la taille de la fenêtre divisée par la taille d’un segment. La fenêtre TCP contrôle directement le débit en limitant le nombre de segments "en vol" simultanément.

---

## Partie 2 — Numéros de séquence

> **Contexte :** Numéros de séquence observés dans la capture :
> 1000 — 2460 — 3920 — 5380 — 6840

### Question 2.1

> _Quelle est la taille d'un segment TCP dans cet exemple ?_

**Réponse :** 2460 - 1000 = **1460 octets** par segment.

---

### Question 2.2

> _Combien d'octets sont transmis entre le premier et le dernier segment ?_

**Réponse :** Il y a 5 segments de 1460 octets chacun : 5 × 1460 = **7300 octets** transmis au total.

---

### Question 2.3

> _Quel sera le prochain numéro de séquence attendu ?_

**Réponse :** 6840 + 1460 = **8300**

---

### Question 2.4

> _Que représente le numéro de séquence dans TCP ?_

**Réponse :** Le numéro de séquence identifie la position du premier octet du segment dans le flux de données. Il permet au destinataire de remettre les segments dans le bon ordre et de détecter les pertes ou doublons.

---

**Réponse :**

---

## Partie 3 — Calcul du débit maximal théorique

> **Contexte :** Taille de fenêtre TCP : 64 KB — RTT : 50 ms
> **Formule :** Débit = TailleFenetre / RTT

### Question 3.1

> _Convertir la taille de fenêtre en octets._

**Réponse :** 64 KB × 1024 = **65 536 octets**

---

### Question 3.2

> _Convertir le RTT en secondes._

**Réponse :** 50 ms ÷ 1000 = **0,05 s**

---

### Question 3.3

> _Calculer le débit maximal théorique en octets par seconde._

**Réponse :** 65 536 / 0,05 = **1 310 720 octets/s**

---

### Question 3.4

> _Convertir le résultat en Ko/s._

**Réponse :** 1 310 720 / 1024 = **1 280 Ko/s**

---

### Question 3.5

> _Convertir le résultat en Mb/s._

**Réponse :** 1 310 720 × 8 = 10 485 760 bits/s ÷ 1 000 000 = **≈ 10,49 Mb/s**

---

## Partie 4 — Influence du RTT sur le débit

> **Contexte :** Fenêtre TCP = 65 536 octets (64 KB) — Formule : Débit = Fenêtre / RTT

### Question 4.1

> _Calculer le débit maximal si le RTT est de 20 ms._

**Réponse :** 65 536 / 0,02 = 3 276 800 octets/s = 3 200 Ko/s ≈ **26,21 Mb/s**

---

### Question 4.2

> _Calculer le débit maximal si le RTT est de 100 ms._

**Réponse :** 65 536 / 0,1 = 655 360 octets/s = 640 Ko/s ≈ **5,24 Mb/s**

---

### Question 4.3

> _Calculer le débit maximal si le RTT est de 200 ms._

**Réponse :** 65 536 / 0,2 = 327 680 octets/s = 320 Ko/s ≈ **2,62 Mb/s**

---

### Question 4.4

> _Que peut-on conclure concernant l'influence du RTT sur les performances TCP ?_

**Réponse :** Le RTT et le débit sont **inversement proportionnels** : doubler le RTT divise le débit par deux. Un RTT élevé dégrade les performances TCP car l'émetteur doit attendre plus longtemps avant de pouvoir envoyer de nouvelles données.

---

## Partie 5 — Fenêtre TCP et acquittements

> **Contexte :** Taille de segment : 1000 octets — Taille de fenêtre : 5000 octets

### Question 5.1

> _Combien de segments peuvent être envoyés avant réception d'un ACK ?_

**Réponse :** 5000 / 1000 = **5 segments** peuvent être envoyés sans attendre d'ACK.

---

### Question 5.2

> _Quels sont les numéros de séquence envoyés si le premier numéro est 0 ?_

**Réponse :** Les 5 segments ont pour numéros de séquence : **0, 1000, 2000, 3000, 4000**

---

### Question 5.3

> _Quel numéro d'acquittement sera envoyé après réception complète des données ?_

**Réponse :** Après réception des 5 segments, le destinataire envoie ACK = **5000** .

---

### Question 5.4

> _Que devient la fenêtre après réception de l'ACK ?_

**Réponse :** La fenêtre **glisse** vers l'avant : l'émetteur peut désormais envoyer 5 nouveaux segments à partir du numéro de séquence 5000.

**Réponse :**

---

## Partie 6 — Analyse d'une capture Wireshark

> **Contexte :** Seq = 0 — Seq = 1460 — Seq = 2920 — Seq = 4380 — Ack = 5840 — Window Size = 5840

### Question 6.1

> _Combien de segments ont été envoyés ?_

**Réponse :** On observe 4 numéros de séquence (0, 1460, 2920, 4380), donc **4 segments** ont été envoyés.

---

### Question 6.2

> _Combien d'octets ont été transmis ?_

**Réponse :** 4 segments × 1460 octets = **5840 octets** transmis.

---

### Question 6.3

> _Pourquoi l'ACK indique 5840 ?_

**Réponse :** Le dernier segment commence à 4380 et fait 1460 octets, il occupe donc les octets 4380 à 5839. L'ACK = 5840 signifie "j'ai bien reçu tout jusqu'à l'octet 5839, envoie-moi à partir de **5840**".

---

### Question 6.4

> _Quelle est la taille de la fenêtre annoncée par le récepteur ?_

**Réponse :** Le champ Window Size indique **5840 octets** — c'est la capacité tampon disponible que le récepteur annonce à l'émetteur pour la prochaine rafale.

---

### Question 6.5

> _Que signifie une diminution de la taille de la fenêtre ?_

**Réponse :** Une fenêtre qui diminue signifie que le tampon du récepteur se remplit. C'est le mécanisme de **contrôle de flux** : le récepteur ralentit l'émetteur pour éviter d'être débordé. Si la fenêtre atteint 0, l'émetteur doit s'arrêter d'envoyer.

**Réponse :**

---

## Partie 7 — Débit réel

> **Contexte :** Taille de fenêtre : 12 000 octets — RTT : 60 ms

### Question 7.1

> _Calculer le débit maximal théorique._

**Réponse :** 12 000 / 0,06 = **200 000 octets/s** = 195,3 Ko/s ≈ **1,6 Mb/s**

---

### Question 7.2

> _Si seulement 8000 octets sont transmis par RTT, quel est le débit réel ?_

**Réponse :** 8 000 / 0,06 ≈ **133 333 octets/s** = 130,2 Ko/s ≈ **1,07 Mb/s**

---

### Question 7.3

> _Comparer débit réel et débit théorique._

**Réponse :** Le débit réel représente **66,7 %** du débit théorique (200 000 o/s). On perd environ 1/3 du débit maximal possible.

---

### Question 7.4

> _Proposer une explication possible à la différence observée._

**Réponse :** Plusieurs causes possibles : congestion réseau, contrôle de flux, overhead des en-têtes TCP/IP, ou limitation applicative.

---

## Partie 8 — Synthèse

### Question 8.1

> _Quel est le rôle principal du sliding window ?_

**Réponse :** Le sliding window permet d'envoyer plusieurs segments en continu sans attendre un ACK pour chacun, maximisant ainsi l'utilisation de la bande passante disponible.

---

### Question 8.2

> _Pourquoi TCP n'envoie-t-il pas les données une par une ?_

**Réponse :** Envoyer un segment puis attendre l'ACK laisserait le réseau inactif pendant toute la durée du RTT. Avec une fenêtre, l'émetteur garde le réseau occupé en permanence et atteint un débit bien plus élevé.

---

### Question 8.3

> _Quel est l'impact d'une petite fenêtre TCP ?_

**Réponse :** Une petite fenêtre limite le nombre de segments en vol simultanément, ce qui réduit directement le débit. L'émetteur se retrouve souvent à attendre des ACK au lieu d'envoyer.

---

### Question 8.4

> _Quel est l'impact d'un RTT élevé ?_

**Réponse :** Un RTT élevé réduit le débit proportionnellement. L'émetteur attend plus longtemps avant de recevoir les ACK et de pouvoir faire glisser la fenêtre.

---

### Question 8.5

> _Pourquoi le mécanisme de sliding window améliore-t-il les performances ?_

**Réponse :** Sans sliding window, chaque segment nécessite un aller-retour complet avant que le suivant parte. Avec sliding window, plusieurs segments s'envoient en parallèle : le réseau est utilisé en continu et le débit est multiplié par le nombre de segments dans la fenêtre.

---
