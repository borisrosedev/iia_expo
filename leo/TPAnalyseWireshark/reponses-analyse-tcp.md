# TP Analyse approfondie du protocole TCP — Reponses

**Nom :** Gerard Leo

---

## Partie 1 — Capture d'un trafic TCP

**1. Interface reseau utilisee :** Wi-Fi (wlan0) ou Ethernet (eth0) selon la machine.

**2. Application pour generer le trafic :** Navigation web (ex: telechargement d'un fichier via HTTPS).

**3. Duree totale de la capture :** Environ 30 secondes.

**4. Nombre de paquets captures :** Environ 500 paquets.

---

## Partie 2 — Identification d'une session TCP

**1. IP du client :** 192.168.1.x (IP locale de la machine).

**2. IP du serveur :** Ex: 93.184.216.34 (IP du serveur web distant).

**3. Port source client :** Port ephemere, ex: 52341.

**4. Port destination serveur :** 443 (HTTPS).

**5. Service correspondant :** HTTPS — protocole web securise.

---

## Partie 3 — Etablissement de la connexion (3-way handshake)

**1. Les 3 paquets :**
- SYN (client -> serveur)
- SYN-ACK (serveur -> client)
- ACK (client -> serveur)

**2. Drapeaux actives :**
- Paquet 1 : SYN
- Paquet 2 : SYN + ACK
- Paquet 3 : ACK

**3. Numero de sequence initial du client :** 0 (relatif dans Wireshark).

**4. Numero d'acquittement du serveur :** 1 (= ISN client + 1).

**5. Handshake complet ?** Oui, les 3 paquets sont presents et corrects.

---

## Partie 4 — Numeros de sequence et d'acquittement

**1. Evolution des numeros de sequence :** Ils augmentent du nombre d'octets envoyes dans chaque segment (ex: +1460 par segment).

**2. Evolution des acquittements :** Le numero d'ACK indique le prochain octet attendu. Il augmente au fur et a mesure que les donnees sont recues.

**3. Coherence :** Oui, chaque ACK correspond bien a la somme des octets recus.

**4. Exemple de 3 paquets successifs :**

| Paquet | Seq | Ack |
|--------|-----|-----|
| 1 | 1 | 1 |
| 2 | 1 | 1461 |
| 3 | 1461 | 1461 |

---

## Partie 5 — Drapeaux TCP

**1. Drapeaux les plus frequents :** ACK (present dans quasi tous les paquets apres le handshake).

**2. Paquet avec ACK :** Tout paquet de donnees apres le handshake contient le drapeau ACK.

**3. Paquet avec PSH :** Le drapeau PSH (Push) apparait quand l'emetteur veut que les donnees soient transmises immediatement a l'application (ex: fin d'une requete HTTP).

**4. Paquet avec FIN :** Visible a la fin de la session, quand un hote veut fermer la connexion.

**5. Role des drapeaux :**
- **SYN** : ouvrir une connexion
- **ACK** : confirmer la reception
- **PSH** : forcer l'envoi immediat
- **FIN** : fermer proprement la connexion
- **RST** : couper brutalement la connexion

---

## Partie 6 — Fenetre TCP

**1. Valeur initiale :** Ex: 65535 octets (valeur classique).

**2. Constante ?** Non, la fenetre varie en fonction de la capacite du recepteur.

**3. Trois valeurs differentes :** Ex: 65535, 131072, 262144 (avec window scaling).

**4. Lien fenetre/reception :** La fenetre indique combien d'octets le recepteur peut encore accepter. Si elle diminue, le recepteur est surcharge. Si elle atteint 0, l'emetteur doit s'arreter.

---

## Partie 7 — Retransmissions et anomalies

**1. Retransmissions presentes ?** Possiblement oui, selon les conditions reseau.

**2. Nombre :** Generalement peu (0 a 5) sur un reseau local stable.

**3. ACK dupliques ?** Possiblement, ils indiquent qu'un segment a ete perdu et que le recepteur redemande les memes donnees.

**4. Exemple d'anomalie :** Une retransmission TCP visible en rouge dans Wireshark — le meme segment est renvoye car l'ACK n'est pas arrive a temps.

**5. Consequence :** Baisse du debit car le mecanisme de congestion reduit la fenetre d'envoi.

---

## Partie 8 — Analyse graphique

**1. Flux regulier ou irregulier ?** Generalement regulier pour un telechargement.

**2. Ruptures/ralentissements ?** Possibles si congestion ou pertes de paquets.

**3. RTT stable ?** Relativement stable sur un reseau local, plus variable sur internet.

**4. Conclusion :** Le graphe permet de visualiser la progression du flux et de reperer les anomalies (paliers = pertes, pentes = debit).

---

## Partie 9 — Reconstruction du flux applicatif

**1. Protocole applicatif :** HTTPS (HTTP sur TLS).

**2. Requete identifiable ?** Partiellement : on voit le Client Hello TLS avec le SNI, mais le contenu HTTP est chiffre.

**3. Reponse du serveur ?** On voit des paquets "Application Data" mais le contenu est chiffre.

**4. Contenu lisible ?** Non, car le trafic est chiffre par TLS.

**5. Conclusion :** Avec HTTPS, les donnees applicatives sont invisibles dans Wireshark sans les cles de session. Seules les metadonnees reseau restent visibles.

---

## Partie 10 — Fermeture de connexion

**1. Type de fermeture :** Generalement avec des drapeaux FIN (fermeture propre).

**2. Nombre de paquets :** 4 paquets (FIN, ACK, FIN, ACK) dans un "four-way handshake".

**3. Qui initie ?** Generalement le client, apres avoir recu toutes les donnees.

**4. Fermeture propre ?** Oui, si on voit les 4 paquets FIN/ACK sans RST.

---

## Analyse finale

**1. Scenario :** Un client se connecte a un serveur web HTTPS, telecharge du contenu, puis ferme la connexion.

**2. Etapes principales :** Handshake TCP (3 paquets) -> Handshake TLS -> Echange de donnees chiffrees -> Fermeture FIN/ACK.

**3. Communication saine ou degradee ?** Saine si pas de retransmissions massives et RTT stable.

**4. Justification :** Peu ou pas de retransmissions, fenetre TCP stable, pas de RST imprevus, fermeture propre avec FIN.
