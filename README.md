# Handshake TLS - Note

## Introduction - Pourquoi TLS ?

Quand un navigateur se connecte à un serveur HTTPS, l'échange de données doit respecter 3 principes :

- **Confidentialité** : Seul ceux autorisé doivent pouvoir voir la donnée
- **Intégrité** : Aucune altération de la donnée
- **Authentification** : L'expéditeur est bien celui qu'il prétend être

**TLS** pour Transport Layer Security est le protocole qui garantit ces trois propriétés.
Il succède à Secure Socket Layer (SSL), obsolète à cause d'une structure incorrect (Exemple : Pas de séparation des clés entre chiffrement et authentification) et de vulnérabilités cryptographiques (Exemple : Attaque POODLE en 2014 ([lien](https://www.oracle.com/security-alerts/poodlecve-2014-3566.html))).
Ce sont les versions **TLS 1.2** (2008) et **TLS 1.3** (2018) qui sont généralement utilisé et recommandé aujourd'hui.

---

## Vue d'ensemble du handshake TLS

Le handshake est la phase de négociation initiale : avant d'échanger la moindre donnée applicative, client et serveur se mettent d'accord sur les algos à utiliser et établissent des clés de session partagées.

Le handshake doit répondre à trois questions :

| Question | Mécanisme |
|---|---|
| Quels algorithmes utiliser ? | Négociation de la cipher suite |
| Êtes-vous bien qui vous prétendez être ? | Certificat X.509 |
| Comment partager un secret sans l'envoyer en clair ? | Échange de clés |

---

## TLS 1.2 - Le handshake en détail

TLS 1.2 prend **2 aller-retours** pour son handshake.

```
Client                                    Serveur
  |                                          |
  |----------- ClientHello ----------------->|
  |<---------- ServerHello ------------------|
  |<---------- Certificate -----------------|
  |<---------- ServerKeyExchange (si ECDHE)--|
  |<---------- ServerHelloDone -------------|
  |                                          |
  |----------- ClientKeyExchange ----------->|
  |----------- ChangeCipherSpec ------------>|
  |----------- Finished -------------------->|
  |                                          |
  |<---------- ChangeCipherSpec ------------|
  |<---------- Finished --------------------|
  |                                          |
  |===== Données applicatives chiffrées ====|
```

### 2.1 ClientHello

Le client envoie :
- La **version TLS** maximale supportée
- Un **Client Random**
- Les **cipher suites** supportées (Les algos)
- Les **extensions** (Exemple : courbes elliptiques supportées...)
Une cipher suite encode 4 informations : algorithme d'échange de clés, algorithme d'authentification, algorithme de chiffrement symétrique et algorithme de hash


### 2.2 ServerHello

Le serveur répond avec :
- La **cipher suite choisie** (ex : `TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384`)
- Un **Server Random**
- Son **Session ID**


### 2.3 Certificate

Le serveur envoie sa **chaîne de certificats X.509**.
Le client valide :
1. La **signature** du certificat par l'autorité de certification (CA)
2. La **date de validité**
3. Le **nom de domaine**
4. La **révocation**

### 2.4 Échange de clés - deux modes :

#### Mode RSA (déprécié en TLS 1.3)

```
Client génère un Pre-Master Secret
→ chiffré avec la clé publique RSA du serveur
→ seul le serveur peut le déchiffrer
```
**Problème** : pas de **Perfect Forward Secrecy**. Si la clé privée est compromise plus tard, toutes les sessions passées peuvent être déchiffrées.

#### Mode ECDHE (recommandé)

```
Client et serveur génèrent chacun une paire de clés éphémère
→ échangent leurs clés publiques éphémères
→ calculent indépendamment le même Pre-Master Secret (protocole Diffie-Hellman)
```
**Avantage** : **PFS** - les clés éphémères sont détruites après la session.

### 2.5 Dérivation des clés de session

À partir du **Pre-Master Secret**, des deux **Randoms** (client + serveur), et d'une Pseudo-Random Function, les deux parties dérivent les mêmes clés :

```
Master Secret = PRF(Pre-Master Secret, "master secret", ClientRandom + ServerRandom)

→ clé de chiffrement client→serveur
→ clé de chiffrement serveur→client
→ clé HMAC client→serveur
→ clé HMAC serveur→client
```

### 2.6 Finished

Chaque partie envoie un message **Finished** contenant un hash de tous les messages du handshake, chiffré avec les nouvelles clés.  
→ Prouve que le handshake n'a pas été altéré (Exemple : protection contre les attaques man-in-the-middle sur la négo).

---

## 3. TLS 1.3 - Les améliorations majeures

TLS 1.3 est une refonte importante de la 1.2, ces objectifs sont d'être plus rapide, sûr et simple

### 3.1 Handshake en 1-RTT (au lieu de 2)

```
Client                                    Serveur
  |                                          |
  |--- ClientHello + key_share ------------->|
  |    (clé publique ECDHE dès le départ)    |
  |                                          |
  |<-- ServerHello + key_share -------------|
  |<-- {Certificate} ----------------------|  ← déjà chiffré
  |<-- {CertificateVerify} ----------------|
  |<-- {Finished} --------------------------|
  |                                          |
  |--- {Finished} ------------------------->|
  |                                          |
  |=== Données applicatives chiffrées ======|
```

Le client envoie sa clé publique ECDHE **dès le ClientHello** → le serveur peut dériver les clés et chiffrer sa réponse immédiatement.

### 3.2 0-RTT (Early Data)

Pour une reconnexion à un serveur déjà connu, TLS 1.3 permet d'envoyer des données dès le **premier message** (avant même la fin du handshake), en utilisant un Pre-Shared Key issu de la session précédente.

> **Attention** : le 0-RTT n'offre pas de protection contre les replay attacks. À n'utiliser qu'avec des requêtes idempotentes (Exemple : GET, pas POST).

### 3.3 Suppression des algorithmes dangereux

TLS 1.3 supprime définitivement :
- RSA comme mécanisme d'échange de clés → **ECDHE obligatoire**
- RC4, DES, 3DES
- MD5, SHA-1 pour les signatures
- La compression (vulnérable à l'attaque CRIME)
- La renégociation

### 3.4 Cipher suites simplifiées

TLS 1.2 : des centaines de combinaisons possibles (dont beaucoup dangereuses).  
TLS 1.3 : seulement **5 cipher suites** standardisées, toutes sûres.

| Cipher suite TLS 1.3 |
|---|
| `TLS_AES_256_GCM_SHA384` |
| `TLS_CHACHA20_POLY1305_SHA256` |
| `TLS_AES_128_GCM_SHA256` |
| `TLS_AES_128_CCM_8_SHA256` |
| `TLS_AES_128_CCM_SHA256` |

### 3.5 Chiffrement du handshake

En TLS 1.2, les certificats et la négociation sont **en clair** (seules les données applicatives sont chiffrées).  
En TLS 1.3, **tout est chiffré dès le ServerHello** → le certificat du serveur n'est plus visible par un attaquant.

### C'est quoi un certificat X.509 ?
 
Un certificat X.509 lie une **identité** (domaine, organisation) à une **clé publique**, le tout signé par une autorité de confiance (CA). C'est ce qui permet à un client TLS de vérifier qu'il parle bien au bon serveur.
 
 
### Structure d'un certificat
 
| Champ | Contenu |
|---|---|
| **Subject** | Identité du propriétaire (`CN=example.com`) |
| **Issuer** | Qui a signé le certificat (`CN=Let's Encrypt`) |
| **Validity** | Dates de début et fin de validité |
| **SAN** | Liste des domaines/IPs couverts (Subject Alternative Name) |
| **Public Key** | La clé publique du serveur |
| **Signature** | Hash du certificat signé par la clé privée du CA |

 
### Chaîne de confiance
 
On ne fait pas confiance directement au certificat du serveur. On remonte une **chaîne** jusqu'à un Root CA connu du navigateur/ de l'OS.
 
```
Root CA
    └── Intermediate CA  (fait le travail quotidien)
            └── Certificat du serveur
```
 
> Les Root CA sont gardés hors-ligne et les intermédiaires peuvent être révoqués sans toucher au Root.

---

## 4. Attaques classiques contre TLS

| Attaque | Cible | Mécanisme | Protection |
|---|---|---|---|
| **BEAST** (2011) | TLS 1.0, mode CBC | En TLS 1.0, le mode CBC utilisait le **dernier bloc chiffré comme IV** pour le bloc suivant, ce qui le rendait prévisible. Un attaquant MITM pouvait soumettre des blocs choisis et déduire le contenu d'un cookie de session octet par octet. | TLS ≥ 1.1, AES-GCM |
| **CRIME** (2012) | Compression TLS | Oracle de compression → fuite de cookies | Désactiver la compression TLS |
| **POODLE** (2014) | SSL 3.0, padding CBC | L'attaquant force une **rétrogradation vers SSL 3.0** en simulant des échecs de connexion TLS, puis exploite le padding CBC mal spécifié de SSL 3.0 pour déchiffrer du contenu | Désactiver SSL 3.0, TLS_FALLBACK_SCSV |
| **Heartbleed** (2014) | OpenSSL (bug implémentation) | Lecture de mémoire de OpenSSL donc fuite de clé privée | Patcher OpenSSL |
| **FREAK** (2015) | Cipher suites "export" RSA | Forcer Cipher suite spécifique dangereuse | Supprimer les suites export |
| **Logjam** (2015) | DH avec paramètres faibles | Forcer Cipher suite spécifique dangereuse  | DH ≥ 2048 bits, ou ECDHE |
| **Downgrade attack** | Négociation de version | MITM modifie les messages du handshake pour faire croire au serveur que le client ne supporte pas les versions récentes, forçant une version vulnérable. | HSTS, TLS_FALLBACK_SCSV, TLS 1.3 |
| **MITM / faux certificat** | Authentification serveur | CA compromise ou attaquant avec un certificat frauduleux | Chaine de certificats valides, DNS CAA, CT logs |

---

## 5. Récapitulatif - Points clés à retenir

```
TLS 1.2 : 2-RTT · RSA ou ECDHE · handshake en clair
TLS 1.3 : 1-RTT · ECDHE obligatoire · handshake chiffré · PFS garanti
```

| Propriété | TLS 1.2 | TLS 1.3 |
|---|---|---|
| Latence | 2-RTT | 1-RTT (0-RTT possible) |
| PFS | Optionnel | Obligatoire |
| Handshake chiffré | Non | Oui |
| Algorithmes faibles | Présents | Supprimés |
| Certificat visible |  En clair | Chiffré |

---

*Sources :*
- RFC 5246 (TLS 1.2)
- RFC 8446 (TLS 1.3)
- ANSSI Guide TLS
- Wikipedia
- IBM - Protocole TLS
