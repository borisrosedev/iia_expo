# Exposé TCP Fast Open
**RSX102 · Elouan Tusseau**

---

## Slide 1 — Titre

Bonjour à tous. Aujourd'hui je vais vous parler de TCP Fast Open, une extension du protocole TCP standardisée en 2014 dans le RFC 7413.

L'idée centrale : envoyer des données applicatives dès le tout premier paquet de la connexion TCP, sans attendre la fin du handshake.

Ça paraît simple, mais ça demande un mécanisme précis — c'est ce qu'on va voir ensemble.

---

## Slide 2 — Le problème

Commençons par le problème que TFO cherche à résoudre.

Quand votre navigateur ouvre une page web, avant même d'envoyer la requête HTTP, TCP doit établir la connexion. Et pour ça, il y a le 3-way handshake : trois échanges obligatoires — SYN, SYN-ACK, ACK. Ce n'est qu'après ces trois étapes que les données peuvent partir.

Sur un RTT de 50 ms — c'est typiquement la latence entre l'Europe et les États-Unis — ça représente 50 ms gaspillées à chaque nouvelle connexion. Sur une connexion longue comme du streaming, ce coût est négligeable. Mais pour des APIs REST, des apps mobiles qui font des dizaines de connexions courtes par session, ça s'accumule rapidement.

D'où la question : est-ce qu'on peut envoyer des données **pendant** le handshake, sans casser la fiabilité de TCP ?

---

## Slide 3 — TCP Classique

Voici le déroulé exact d'une connexion TCP classique.

Le client envoie un SYN — *je veux me connecter*. Le serveur répond SYN-ACK — *OK je suis là*. Le client confirme avec un ACK. C'est le premier RTT, entièrement consacré à la synchronisation.

Ce n'est qu'après ces trois étapes que le client peut envoyer sa requête HTTP. Puis il attend la réponse — deuxième RTT.

Donc avant même de recevoir la première réponse utile, on a consommé au minimum deux RTT. Pour des connexions fréquentes et courtes, c'est un goulot d'étranglement réel.

C'est exactement ce que TCP Fast Open cherche à éliminer.

---

## Slide 4 — TFO Principe

TCP Fast Open casse cette règle : les données partent dès le SYN.

Au lieu d'envoyer un SYN vide, le client envoie SYN + ses données applicatives directement. Le serveur valide et répond avec SYN-ACK + sa réponse. Le client confirme avec ACK.

Résultat : on passe de deux RTT à un seul. Google a mesuré en 2014 une réduction de 10 % de la latence en médiane, et jusqu'à 40 % pour les connexions les plus lentes.

Mais comment le serveur sait-il que ce client est légitime et qu'il peut traiter ses données sans attendre la fin du handshake ? C'est là qu'intervient le **cookie TFO**.

---

## Slide 5 — Phase 1 : Acquisition du cookie

Le mécanisme fonctionne en deux phases.

La première connexion est une connexion standard. Le client ajoute simplement une option TCP spéciale — *"donne-moi un cookie TFO"*. Le serveur génère un cookie chiffré à partir de l'adresse IP du client et de sa propre clé secrète, puis le renvoie dans le SYN-ACK.

Le client stocke ce cookie localement. Cette première connexion ne bénéficie d'**aucun gain de latence** — elle sert uniquement à initialiser.

Deux points importants : le cookie est lié à l'adresse IP du client, donc il ne peut pas être utilisé depuis une autre machine. Et il a une durée de validité limitée.

---

## Slide 6 — Phase 2 : TFO actif

À partir de la deuxième connexion, TFO est actif.

Le client envoie le SYN avec son cookie TFO **et** ses données applicatives dans le même paquet. Le serveur vérifie le cookie — si c'est valide, il passe les données directement à l'application et répond immédiatement avec SYN-ACK + sa réponse. Un seul RTT.

Point important : si le cookie est invalide ou expiré, le serveur ignore les données du SYN et retombe sur un handshake classique. L'utilisateur ne voit rien, pas d'erreur. C'est ce qu'on appelle la **dégradation gracieuse**.

---

## Slide 7 — Avantages

En résumé, TFO apporte quatre avantages principaux.

La **latence** : une économie d'un RTT par connexion, directement mesurable.

La **compatibilité** : TFO ne touche pas aux mécanismes de contrôle de congestion existants — Reno, CUBIC, BBR — rien ne change côté gestion du débit.

La **sécurité** : le cookie chiffré protège contre le spoofing IP. Sans avoir reçu la réponse du serveur sur sa vraie IP, impossible de fabriquer un cookie valide.

Et la **robustesse** avec la dégradation gracieuse.

Les chiffres Google sont frappants surtout sur le 1er percentile — les 40 % de gain concernent les connexions les plus lentes, souvent des utilisateurs sur des réseaux difficiles, en mobilité.

---

## Slide 8 — Limitations

TFO a quatre limitations concrètes.

**01 —** Pas de gain sur la toute première connexion — le client n'a pas encore de cookie.

**02 —** Les **middleboxes**. C'est le problème principal en production. Les pare-feux, NAT et proxies entre le client et le serveur ne comprennent pas forcément les options TCP de TFO. Certains suppriment les données du SYN, d'autres bloquent la connexion. C'est le même problème qui a poussé Google à développer QUIC sur UDP.

**03 —** Le **risque de rejeu**. Le réseau peut retransmettre un SYN automatiquement si l'ACK tarde — ce qui fait que le serveur reçoit deux fois les mêmes données. TFO impose donc que les opérations soient idempotentes : un GET peut être rejoué sans problème, un POST qui crée une commande ne doit pas l'être.

**04 —** TFO est inutile sur des connexions longues comme SSH ou du streaming, où le coût du handshake initial est marginal.

---

## Slide 9 — Support

Côté support, TFO est bien implanté aujourd'hui.

Linux le supporte nativement depuis le noyau 3.7 en 2012 — avant même la publication du RFC officiel en 2014. L'activation se fait avec une simple commande : `echo 3` dans le fichier `tcp_fastopen` de `/proc`. La valeur 3 active TFO à la fois côté client et côté serveur.

Android le supporte depuis Android 5, Windows depuis Windows 10 v1709, et les navigateurs modernes le gèrent en général.

Le vrai frein au déploiement n'est pas le support logiciel — c'est la présence de middleboxes sur le réseau qui peuvent bloquer TFO de manière silencieuse.

---

## Slide 10 — Écosystème

Pour situer TFO dans le temps.

En **1981**, TCP classique impose au moins deux RTT avant la première donnée utile.

En **2014**, TCP Fast Open réduit ça à un RTT — c'est là où on est dans cet exposé.

En **2018**, TLS 1.3 introduit le mode 0-RTT : si le client s'est déjà connecté, il peut envoyer des données chiffrées dès le premier message TLS. Même principe que TFO, mais au niveau cryptographique.

En **2021**, QUIC — la base de HTTP/3 — résout le problème de façon radicale en partant d'UDP. Zéro RTT natif, plus de problème de middleboxes.

TFO est une étape intermédiaire pragmatique : il optimise TCP sans le remplacer. QUIC va plus loin mais avec une rupture de compatibilité bien plus forte.

---

## Slide 11 — Conclusion

Pour conclure.

TCP Fast Open c'est simple en principe : un cookie chiffré permet d'envoyer des données dès le premier paquet SYN, économisant un RTT par connexion. Le gain est réel et mesurable sur des connexions courtes et fréquentes.

Les deux points à retenir côté limitation : les **middleboxes** en production, et l'**idempotence** des opérations envoyées dans le SYN.

TFO s'inscrit dans une tendance plus large de réduction de la latence — il est complémentaire à TLS 1.3 et a inspiré directement les mécanismes de QUIC.

Je suis disponible pour les questions.
