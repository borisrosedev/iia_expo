# TP Packet Tracer — Reponses

**Nom :** Gerard Leo

---

## Questions

**1. Qu'est-ce qu'un systeme distribue ?**

C'est un ensemble de machines independantes qui travaillent ensemble pour fournir un service. L'utilisateur voit un seul service, mais en realite plusieurs machines se partagent le travail.

**2. Pourquoi utiliser plusieurs serveurs ?**

Pour eviter que le service s'arrete si un seul serveur tombe en panne. Avec plusieurs serveurs, si l'un est hors service, les autres prennent le relais.

**3. Qu'est-ce qu'un SPOF (Single Point Of Failure) ?**

C'est un composant unique dont la panne entraine l'arret complet du service. Exemple : si on n'a qu'un seul switch et qu'il tombe en panne, plus rien ne fonctionne. La redondance sert justement a eliminer les SPOF.

**4. Pourquoi la redondance ameliore la cybersecurite ?**

Parce qu'un attaquant (ex: attaque DDoS) devrait mettre hors service TOUS les serveurs redondants pour interrompre le service. C'est beaucoup plus difficile que de cibler un seul point de defaillance.

**5. Que se passe-t-il si un switch tombe en panne ?**

Grace a la redondance (2 switchs relies entre eux), le trafic passe par le second switch. Les machines connectees au switch en panne peuvent toujours communiquer via le chemin alternatif.

**6. Lien entre haute disponibilite et cybersecurite ?**

La haute disponibilite garantit que le service reste accessible en permanence. C'est un pilier de la cybersecurite (le "A" de la triade CIA : Confidentialite, Integrite, **Availability/Disponibilite**). Un service indisponible est une faille de securite.
