# TP Minikube — Reponses

**Nom :** Gerard Leo

---

## Questions

**1. Difference entre systeme centralise et distribue ?**

- **Centralise** : une seule machine fait tout. Si elle tombe, tout s'arrete.
- **Distribue** : plusieurs machines travaillent ensemble. Si une tombe, les autres prennent le relais.

**2. Pourquoi plusieurs instances d'une application ?**

Pour la disponibilite : si une instance plante, les autres continuent a repondre. Ca permet aussi de repartir la charge entre plusieurs machines.

**3. Que se passe-t-il si un pod tombe en panne ?**

Kubernetes detecte la panne et recree automatiquement un nouveau pod pour remplacer celui qui est mort. C'est le principe d'**auto-guerison** (self-healing).

**4. Qu'est-ce que la tolerance aux fautes ?**

C'est la capacite d'un systeme a continuer de fonctionner meme quand un de ses composants tombe en panne. Le service reste disponible malgre la defaillance.

**5. Kubernetes garantit-il la haute disponibilite ?**

Oui, grace a la replication des pods et l'auto-guerison. Kubernetes surveille l'etat des pods en permanence et les recree si necessaire. Tant qu'il reste des pods actifs, le service reste accessible.

**6. Quel est le role du load balancing ?**

Le load balancing (equilibrage de charge) repartit les requetes des utilisateurs entre les differentes instances de l'application. Ca evite de surcharger une seule instance et ca permet d'utiliser toutes les ressources disponibles.

---

## Bonus

**Plus il y a de replicas, plus le systeme est fiable ?**

Oui, jusqu'a un certain point. Plus il y a de replicas, plus il faut de pannes simultanees pour interrompre le service. Mais il y a des rendements decroissants : passer de 1 a 3 replicas change beaucoup, passer de 10 a 12 change peu. De plus, trop de replicas consomme des ressources inutilement.
