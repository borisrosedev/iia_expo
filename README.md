# TCP CUBIC
### Contrôle de congestion pour les réseaux modernes
**Boris Rose — RSX102 : Technologies pour les applications en réseau**

---

## Sommaire

1. [Rappels — Contrôle de congestion TCP](#1-rappels--contrôle-de-congestion-tcp)
2. [Limites des algorithmes classiques](#2-limites-des-algorithmes-classiques)
3. [BIC-TCP : l'ancêtre direct de CUBIC](#3-bic-tcp--lancêtre-direct-de-cubic)
4. [CUBIC — Définition et contexte](#4-cubic--définition-et-contexte)
5. [La fonction cubique : le cœur de l'algorithme](#5-la-fonction-cubique--le-cœur-de-lalgorithme)
6. [Les quatre principes de conception (RFC 8312)](#6-les-quatre-principes-de-conception-rfc-8312)
7. [Comportement de la fenêtre : les deux phases](#7-comportement-de-la-fenêtre--les-deux-phases)
8. [Mode TCP-Friendly](#8-mode-tcp-friendly)
9. [Équité entre flux — RTT-Fairness](#9-équité-entre-flux--rtt-fairness)
10. [Déploiement et support système](#10-déploiement-et-support)
11. [Comparaison CUBIC vs BBR](#11-comparaison-cubic-vs-bbr)
12. [Conclusion](#12-conclusion)
13. [Glossaire](#glossaire)

---

## 1. Rappels — Contrôle de congestion TCP

Le **contrôle de congestion** est le mécanisme par lequel TCP adapte son débit d'envoi pour éviter de saturer le réseau. Il repose sur une variable clé : la **fenêtre de congestion (cwnd)**.

Les algorithmes classiques incluent :

| Phase | Comportement |
|---|---|
| **Slow Start** | Croissance exponentielle de cwnd |
| **Congestion Avoidance** | Croissance linéaire (AIMD) |
| **Fast Retransmit** | Retransmission immédiate sur 3 ACK dupliqués |
| **Fast Recovery** | Réduction de cwnd sans revenir à Slow Start |

> **Principe AIMD** : Additive Increase, Multiplicative Decrease — augmentation progressive, réduction brutale en cas de perte.

---

## 2. Limites des algorithmes classiques

Reno et New Reno fonctionnent bien sur des réseaux à **faible bande passante et faible latence**. Mais Internet a évolué vers des réseaux à très haute capacité (fibre optique, liens longue distance), appelés **Long Fat Networks (LFN)**.

**Problème de Reno sur un LFN :**

- Le RTT est élevé → les ACKs arrivent lentement
- La fenêtre cwnd augmente lentement (1 MSS par RTT en phase linéaire)
- La capacité du lien n'est jamais pleinement exploitée
- Il faut des centaines de RTT pour récupérer après une perte

**Iniquité entre flux :**  
Un flux avec un RTT court reçoit les ACKs plus rapidement → sa fenêtre croît plus vite → il capte une plus grande part de la bande passante. Ce comportement est **injuste** par rapport aux flux à long RTT.

---

## 3. BIC-TCP : l'ancêtre direct de CUBIC

**BIC (Binary Increase Congestion Control)** est apparu vers 2004 et est devenu le défaut des noyaux Linux 2.6.8 à 2.6.18.

Son idée clé : utiliser une **recherche binaire** entre la dernière valeur safe (avant congestion) et une valeur cible plus haute pour explorer la bande passante disponible.

**Limites de BIC :**
- Croissance en escalier (fonction linéaire par morceaux) → comportement **non lisse**
- Trop agressif sur certains réseaux
- Complexité d'implémentation

CUBIC est conçu pour corriger ces défauts tout en conservant les forces de BIC.

---

## 4. CUBIC — Définition et contexte

**CUBIC** est un algorithme de contrôle de congestion pour TCP défini dans la **RFC 8312** (février 2018). C'est le successeur de BIC-TCP et **l'algorithme par défaut dans Linux depuis le noyau 2.6.19** (novembre 2006).

> CUBIC modifie uniquement **la fonction de croissance de la fenêtre côté émetteur**, sans toucher au reste du protocole TCP.

**Adoption mondiale :**
- Linux (par défaut depuis 2006)
- Windows 10 Fall Creators Update (2017)
- Windows Server 2016 (mise à jour 1709) et Server 2019
- FreeBSD depuis la version 14.x

**Auteurs :** Sangtae Ha, Injong Rhee (NCSU) et Lisong Xu — article publié dans ACM SIGOPS Operating Systems Review (2008).

---

## 5. La fonction cubique : le cœur de l'algorithme

CUBIC remplace la **croissance linéaire** de la fenêtre par une **fonction cubique** du temps écoulé depuis le dernier événement de congestion :

```
W(t) = C × (t − K)³ + W_max
```

Où :
- **W(t)** = taille de la fenêtre à l'instant t
- **C** = constante d'échelle (contrôle l'agressivité)
- **K** = instant où W(t) = W_max (point d'inflexion)
- **W_max** = taille de la fenêtre au moment de la dernière perte
- **t** = temps écoulé depuis la dernière perte

**Propriété clé de la fonction cubique :**  
La courbe en S présente une **phase concave** (croissance rapide puis ralentissement) suivie d'une **phase convexe** (lente reprise puis accélération).

---

## 6. Les quatre principes de conception (RFC 8312)

**Principe 1 — Utilisation et stabilité**  
CUBIC utilise les deux profils (concave ET convexe) de la fonction cubique, contrairement aux algorithmes qui n'utilisent que le profil convexe. Cela réduit les bursts de paquets autour du point de saturation.

**Principe 2 — TCP-Friendly**  
Sur les réseaux à RTT court et faible bande passante (où TCP standard est efficace), CUBIC se comporte **comme TCP standard** pour ne pas le pénaliser.

**Principe 3 — RTT-Fairness**  
La croissance de la fenêtre est basée sur le **temps réel** (et non sur les ACKs reçus), ce qui la rend **indépendante du RTT**. Résultat : deux flux avec des RTT différents obtiennent la même part de bande passante.

**Principe 4 — Facteur de réduction multiplicative**  
CUBIC calibre son facteur β de réduction pour équilibrer vitesse de convergence et scalabilité.

---

## 7. Comportement de la fenêtre : les deux phases

### Phase concave (retour vers W_max)

Après une perte de paquet :
1. CUBIC enregistre **W_max** (la taille de fenêtre au moment de la perte)
2. Réduit cwnd de façon multiplicative (β ≈ 0.7)
3. Recommence à augmenter cwnd **rapidement d'abord**, puis **de plus en plus lentement** à l'approche de W_max

→ La croissance ralentit car on est proche du seuil de congestion précédent → comportement prudent et stable.

### Phase convexe (exploration au-delà de W_max)

Une fois W_max atteint sans nouvelle perte :
1. CUBIC suppose que le réseau a peut-être évolué (plus de bande passante disponible)
2. La fenêtre augmente **lentement au début**, puis de plus en plus vite
3. Exploration agressive pour trouver un nouveau point de saturation

→ La fenêtre reste longtemps proche de W_max (plateau) → **utilisation maximale et stable du réseau**.

```
cwnd
  ^
  |          W_max             nouveau W_max
  |    ___________..........._____________
  |   /           (plateau)               \___..
  |  /                                         \
  | /
  |/
  +-----------------------------------------> temps
       perte      concave | convexe
```

---

## 8. Mode TCP-Friendly

Sur les petits réseaux (RTT ≤ quelques ms, faible bande passante), CUBIC entre automatiquement en **mode TCP-Friendly** :

- Il calcule la fenêtre qu'aurait TCP standard dans les mêmes conditions
- Si cette valeur est **supérieure** à ce que la fonction cubique donnerait, CUBIC adopte la valeur TCP standard
- Cela garantit que CUBIC ne pénalise pas les flux TCP classiques avec lesquels il coexiste

> CUBIC est dit **TCP-friendly** : il partage équitablement la bande passante avec des flux Reno ou New Reno.

---

## 9. Équité entre flux — RTT-Fairness

Différence fondamentale avec TCP Reno :

| Algorithme | Base de croissance | RTT court | RTT long |
|---|---|---|---|
| **Reno** | Nombre d'ACKs reçus | Avantage fort | Pénalisé |
| **CUBIC** | Temps réel écoulé | Neutre | Neutre |

CUBIC ajuste sa fenêtre à **intervalles réguliers basés sur le temps**, indépendamment de la fréquence d'arrivée des ACKs. Deux flux — l'un sur un lien à 10ms de RTT, l'autre à 200ms de RTT — obtiennent ainsi des parts équivalentes de bande passante.

---

## 10. Déploiement et support

| Système | Statut |
|---|---|
| Linux ≥ 2.6.19 | Par défaut (depuis 2006) |
| Windows 10 (v1709+) | Intégré |
| Windows Server 2016/2019 | Intégré |
| FreeBSD ≥ 14.x | Par défaut |
| macOS | New Reno (pas CUBIC natif) |

**Vérification sur Linux :**
```bash
sysctl net.ipv4.tcp_congestion_control
# → cubic

# Lister les algorithmes disponibles
sysctl net.ipv4.tcp_available_congestion_control
# → reno cubic bbr ...

# Changer temporairement
sysctl -w net.ipv4.tcp_congestion_control=bbr
```

---

## 11. Comparaison CUBIC vs BBR

| Critère | CUBIC | BBR |
|---|---|---|
| **Base** | Perte de paquets | Modèle bande passante + RTT |
| **Comportement en cas de perte** | Réduit cwnd | Continue à estimer le débit optimal |
| **Réseau à pertes aléatoires** | Peut sous-performer | Meilleure résistance |
| **Équité entre flux** | Bonne (RFC 8312) | Contestée (BBRv1) |
| **Adoption** | Très large (défaut Linux) | Croissante (YouTube, QUIC) |
| **Complexité** | Modérée | Élevée |

> CUBIC reste la référence absolue sur Internet par sa **stabilité prouvée** et son **déploiement massif**. BBR est plus performant dans certains contextes (réseaux à forte perte ou très haute latence) mais son équité est encore discutée.

---

## 12. Conclusion

**Pourquoi CUBIC est-il dominant ?**

- Il résout les **deux grandes limites** de Reno : sous-utilisation des LFN et iniquité entre flux de RTT différents
- Il est **rétrocompatible** avec TCP standard (mode TCP-Friendly)
- Il est **stable** et prouvé en production depuis 2006 sur des milliards de connexions
- Sa **formule mathématique élégante** (une seule équation cubique) offre un comportement prévisible

**À retenir :**

> CUBIC est l'exemple parfait d'une optimisation ciblée : changer **une seule chose** (la fonction de croissance de la fenêtre) pour résoudre des problèmes fondamentaux de performance et d'équité, sans toucher au reste du protocole.

---

## Glossaire

### Protocoles et algorithmes

**TCP (Transmission Control Protocol)**
Protocole de transport fiable de la couche 4 du modèle OSI. Il garantit la livraison ordonnée et sans erreur des données grâce aux accusés de réception, numéros de séquence et mécanismes de retransmission.

**TCP Reno**
Variante de TCP introduite en 1990 qui implémente l'algorithme AIMD : la fenêtre augmente linéairement tant qu'il n'y a pas de perte, puis est divisée par 2 dès qu'une perte est détectée.

**New Reno**
Amélioration de Reno (1996) qui gère mieux les pertes multiples.

**TCP Tahoe**
Premier algorithme de contrôle de congestion TCP.

**BIC-TCP**
Algorithme basé sur une recherche binaire pour explorer la bande passante.

**CUBIC**
Algorithme basé sur une fonction cubique du temps pour faire évoluer la fenêtre de congestion.

**BBR**
Algorithme basé sur un modèle du réseau (bande passante + RTT).

**QUIC**
Protocole de transport moderne basé sur UDP avec TLS intégré.

---

### Mécanismes TCP

**AIMD (Additive Increase, Multiplicative Decrease)**
Augmentation progressive de la fenêtre, diminution brutale en cas de perte.

**Slow Start**
Phase initiale où la fenêtre double à chaque RTT.

**Congestion Avoidance**
Phase de croissance linéaire pour éviter la saturation.

**Fast Retransmit**
Retransmission rapide après 3 ACK dupliqués.

**Fast Recovery**
Phase permettant de continuer sans revenir à Slow Start.

**ACK (Acknowledgment)**
Accusé de réception envoyé par le récepteur.

**ACK dupliqué**
ACK répété indiquant une perte de paquet.

**Timeout (RTO)**
Délai après lequel un paquet est retransmis sans ACK.

---

### Variables TCP

**cwnd (Congestion Window)**
Nombre d'octets pouvant être envoyés sans ACK.

**ssthresh**
Seuil entre Slow Start et Congestion Avoidance.

**MSS**
Taille maximale d'un segment TCP.

**RTT**
Temps aller-retour d'un paquet.

**β (beta)**
Facteur de réduction de la fenêtre après congestion.

**W_max**
Fenêtre maximale atteinte avant perte.

**BDP**
Produit bande passante × délai.

---

### Concepts réseau

**Latence**
Temps de transmission d'un paquet.

**Bande passante**
Capacité maximale du réseau.

**Débit (Throughput)**
Vitesse réelle de transmission.

**Perte de paquet**
Paquet non reçu.

**File d'attente (Queue)**
Zone de stockage temporaire dans les routeurs.

**Burst**
Envoi massif de paquets en peu de temps.

**Flux (flow)**
Suite de paquets entre deux hôtes.

**Point de saturation**
Limite où le réseau commence à perdre des paquets.

---

### Concepts mathématiques (CUBIC)

**Fonction concave**
Croissance rapide puis ralentissement.

**Fonction convexe**
Croissance lente puis accélération.

**Point d'inflexion**
Changement de forme de la courbe.

**Plateau**
Zone de stabilité autour de W_max.

---

### Propriétés des algorithmes

**RTT-Fairness**
Équité entre flux indépendamment du RTT.

**TCP-Friendly**
Compatibilité avec TCP Reno.

**Scalabilité**
Capacité à fonctionner efficacement sur de grands réseaux.

---

### Standards et systèmes

**RFC**
Documents définissant les standards Internet.

**IETF**
Organisation qui publie les RFC.

**Noyau Linux**
Cœur du système Linux.

**FreeBSD**
Système UNIX.

**sysctl**
Outil pour modifier les paramètres du noyau.

