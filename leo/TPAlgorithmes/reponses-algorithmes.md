# TP Algorithmes fondamentaux — Reponses

**Nom :** Gerard Leo

---

## Exercice 1 : Horloges de Lamport

> Calculer la valeur de l'horloge logique pour chaque evenement.

Regle : a chaque evenement, on incremente de 1. A la reception, on prend le max entre son horloge et celle du message, puis +1.

| Evenement | Processus | Horloge |
|-----------|-----------|---------|
| 1. Event local | P1 | 1 |
| 2. Envoi msg | P1 | 2 |
| 3. Reception msg | P2 | 3 |
| 4. Envoi msg | P2 | 4 |
| 5. Reception msg | P3 | 5 |

- P1 : 0 -> 1 (event local) -> 2 (envoi)
- P2 : 0 -> max(0, 2)+1 = 3 (reception) -> 4 (envoi)
- P3 : 0 -> max(0, 4)+1 = 5 (reception)

---

## Exercice 2 : Horloges vectorielles

> Donner l'etat du vecteur d'horloge apres chaque evenement.

Regle : on incremente sa propre composante. A la reception, on prend le max composante par composante, puis on incremente.

| Evenement | P1 | P2 | P3 |
|-----------|----|----|-----|
| Initial | (0,0,0) | (0,0,0) | (0,0,0) |
| 1. P1 event local | **(1,0,0)** | (0,0,0) | (0,0,0) |
| 2. P1 envoie a P2 | **(2,0,0)** | (0,0,0) | (0,0,0) |
| 3. P2 recoit | (2,0,0) | **(2,1,0)** | (0,0,0) |
| 4. P2 event local | (2,0,0) | **(2,2,0)** | (0,0,0) |

---

## Exercice 3 : Algorithme Bully

**1. A quels processus P2 envoie-t-il un message ?**

P2 envoie un message d'election a P3 et P4 (tous ceux avec un ID superieur). P4 ne repond pas (en panne). P3 repond "OK".

**2. Quel processus devient le nouveau leader ?**

**P3** devient le nouveau leader car c'est le processus actif avec l'ID le plus eleve.

**3. Pourquoi "Bully" ?**

Parce que le processus avec le plus grand ID "intimide" les autres et prend le pouvoir. C'est la loi du plus fort.

---

## Exercice 4 : Consensus distribue

**1. Quelle valeur est choisie ?**

**10** est choisie (2 votes contre 1 pour 20).

**2. Que se passe-t-il si S2 tombe en panne ?**

Il reste S1 (10) et S3 (20). Pas de majorite claire. Le consensus ne peut pas etre atteint de maniere fiable avec seulement 2 serveurs sur 3.

**3. Combien de serveurs minimum pour tolerer une panne ?**

**3 serveurs** minimum pour tolerer 1 panne. Formule : 2f+1 serveurs pour tolerer f pannes. Avec 3 serveurs, on peut perdre 1 et garder une majorite (2/3).

---

## Exercice 5 : Exclusion mutuelle distribuee

> Determiner l'ordre d'acces a la ressource.

On classe par timestamp croissant :

1. **P2** (timestamp 2) - accede en premier
2. **P1** (timestamp 5) - accede en deuxieme
3. **P3** (timestamp 8) - accede en dernier

Le processus avec le plus petit timestamp a la priorite.

---

## Exercice 6 : Replication et quorum

**1. Peut-on lire si 3 serveurs sont en panne ?**

Oui. Il reste 2 serveurs actifs (5-3=2), et la lecture necessite 2 reponses. C'est juste suffisant.

**2. Peut-on ecrire si 2 serveurs sont en panne ?**

Oui. Il reste 3 serveurs actifs (5-2=3), et l'ecriture necessite 3 confirmations. C'est juste suffisant.

**3. Pourquoi l'ecriture necessite plus de confirmations ?**

Pour garantir la coherence : il faut que toute lecture future puisse voir la derniere ecriture. Si R (lecture) + W (ecriture) > N (total serveurs), alors lecture et ecriture se "chevauchent" forcement sur au moins un serveur. Ici : 2 + 3 = 5 = N, donc c'est garanti.

---

## Exercice 7 : Detection de panne par heartbeat

> Apres combien de temps peut-on suspecter une panne ?

Le dernier "alive" arrive a **4s**. Le suivant aurait du arriver a 6s mais rien. On peut suspecter la panne a partir de **6s** (premier heartbeat manque).

En pratique, on attend souvent 2 heartbeats manques pour eviter les faux positifs, donc a **8s** on peut confirmer la panne avec plus de certitude.
