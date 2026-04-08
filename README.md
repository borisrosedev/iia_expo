# TCP CUBIC — Exposé RSX102
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

## Glossaire — TCP CUBIC
---

### Protocoles et algorithmes

**TCP (Transmission Control Protocol)**
Protocole de transport fiable de la couche 4 du modèle OSI. Il garantit la livraison ordonnée et sans erreur des données grâce aux accusés de réception, numéros de séquence et mécanismes de retransmission.

---

**TCP Reno**
Variante de TCP introduite en 1990 qui implémente l'algorithme AIMD : la fenêtre augmente linéairement tant qu'il n'y a pas de perte, puis est divisée par 2 dès qu'une perte est détectée.

---

**New Reno**
Amélioration de Reno (1996) qui gère mieux les pertes multiples dans une même fenêtre : au lieu de sortir immédiatement de Fast Recovery, New Reno continue à retransmettre les segments manquants un par un.

---

**TCP Tahoe**
Premier algorithme de contrôle de congestion de TCP (Van Jacobson, 1988). En cas de perte, il remet cwnd à 1 et repart en Slow Start, sans Fast Recovery.

---

**BIC-TCP (Binary Increase Congestion Control)**
Prédécesseur de CUBIC, défaut de Linux de 2004 à 2006. Il utilise une recherche binaire entre la dernière valeur "sûre" de cwnd et une valeur cible plus haute pour explorer la bande passante disponible. Sa croissance est linéaire par morceaux, ce qui la rend non lisse.

---

**BBR (Bottleneck Bandwidth and RTT)**
Algorithme développé par Google, basé non pas sur les pertes mais sur un modèle du réseau : il estime en permanence la bande passante maximale disponible et le RTT minimal, puis ajuste son débit en conséquence. Utilisé notamment par YouTube et QUIC.

---

**QUIC**
Protocole de transport développé par Google, fonctionnant au-dessus d'UDP avec TLS 1.3 intégré. C'est la base de HTTP/3. Il gère nativement le multiplexage sans head-of-line blocking.

---

### Variables et mécanismes TCP

**cwnd (Congestion Window)**
Fenêtre de congestion. Variable maintenue par l'émetteur qui limite le nombre d'octets pouvant être envoyés sans avoir reçu d'accusé de réception. Elle est ajustée dynamiquement par l'algorithme de contrôle de congestion.

---

**ssthresh (Slow Start Threshold)**
Seuil de démarrage lent. Valeur de cwnd à partir de laquelle TCP passe de la croissance exponentielle (Slow Start) à la croissance linéaire (Congestion Avoidance). Initialement grand, il est réduit de moitié à chaque détection de congestion.

---

**ACK (Acknowledgment)**
Accusé de réception. Segment envoyé par le récepteur pour confirmer qu'il a bien reçu des données. En TCP classique, chaque ACK reçu par l'émetteur déclenche une augmentation de cwnd.

---

**MSS (Maximum Segment Size)**
Taille maximale des données dans un segment TCP, hors en-têtes. Typiquement 1460 octets sur Ethernet (MTU de 1500 octets − 20 octets d'en-tête IP − 20 octets d'en-tête TCP). Négocié lors du handshake.

---

**RTT (Round Trip Time)**
Temps aller-retour. Durée mesurée entre l'envoi d'un paquet et la réception de son accusé de réception. C'est la métrique fondamentale pour estimer les délais et calibrer les retransmissions dans TCP.

---

**β (beta)**
Facteur de réduction multiplicative de cwnd lors d'une congestion. Dans CUBIC, β ≈ 0,7 (cwnd est réduit à 70 % de sa valeur), contre 0,5 dans Reno (division par 2). Ce réglage plus doux permet une récupération plus rapide.

---

**W_max**
Valeur de cwnd au moment où la dernière perte de paquet a été détectée. C'est le point d'inflexion de la fonction cubique de CUBIC : la courbe converge vers cette valeur puis repart à l'exploration au-delà.

---

**BDP (Bandwidth-Delay Product)**
Produit bande passante × délai. Mesure la quantité de données "en vol" dans le réseau à un instant donné. Un BDP élevé caractérise un Long Fat Network et nécessite une grande fenêtre de congestion pour exploiter pleinement le lien.

---

### Concepts réseau

**Long Fat Network (LFN)**
Réseau à fort BDP, c'est-à-dire à la fois haut débit et haute latence (ex. fibre intercontinentale, liens satellite). Les algorithmes comme Reno y sont inefficaces car leur croissance linéaire est trop lente pour remplir le "tuyau".

---

**Flux (flow)**
Séquence de paquets échangés entre deux hôtes identifiés par le même quintuplet (IP source, IP destination, port source, port destination, protocole). Plusieurs flux peuvent partager un même lien physique.

---

**Burst**
Pic d'envoi : l'émetteur envoie en rafale un grand nombre de paquets en très peu de temps, ce qui peut saturer les files d'attente des routeurs et provoquer des pertes.

---

**Point de saturation**
Valeur de cwnd à partir de laquelle le réseau ne peut plus absorber davantage de données sans commencer à perdre des paquets ou à allonger ses files d'attente.

---

### Standards et institutions

**RFC (Request For Comments)**
Documents publiés par l'IETF qui définissent les standards d'Internet. La RFC 8312 est la spécification officielle de CUBIC ; la RFC 2018 définit SACK, etc.

---

**IETF (Internet Engineering Task Force)**
Organisation internationale qui développe et publie les standards techniques d'Internet sous forme de RFC. Ouverte et collaborative, elle fonctionne par consensus.

---

**ACM SIGOPS**
Revue académique de l'Association for Computing Machinery dédiée aux systèmes d'exploitation. L'article fondateur de CUBIC par Ha, Rhee et Xu y a été publié en 2008.

---

### Commandes et systèmes

**sysctl**
Outil Linux permettant de lire et modifier des paramètres du noyau en temps réel, sans redémarrage. `net.ipv4.tcp_congestion_control` est le paramètre qui détermine quel algorithme de congestion TCP est actif.

---

**FreeBSD**
Système d'exploitation de type UNIX, distinct de Linux, réputé pour sa stabilité et sa robustesse. Il utilise CUBIC comme algorithme de congestion par défaut depuis sa version 14.x.

---

**Noyau Linux (kernel)**
Le cœur du système d'exploitation Linux, qui gère notamment la pile réseau TCP/IP. CUBIC y est implémenté et activé par défaut depuis la version 2.6.19 (novembre 2006).

*Sources : RFC 8312, Ha et al. (2008) ACM SIGOPS, Wikipedia CUBIC TCP, cours de Boris Rose*
