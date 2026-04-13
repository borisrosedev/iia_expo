# Algorithme de Ricart-Agrawala


## Exclusion Mutuelle

L'algorithme de Ricart-Agrawala est un algorithme d'exclusion mutuelle sur un système distribué. 
L'exclusion mutuelle (ou mutual exclusion, réduit à mutex car les caractères ça coûte cher (Looking at you, North America!)) est le processus par lequel un environnement informatique assure que plusieurs programmes n'accède pas simultanément à une même ressource. 



### Implémentations et problèmes possibles

Le mutex peut être implémenter de plusieurs façons selon les besoins du système l'hébergeant: Il peux par exemple limiter l'écriture d'une ressource à un processus mais permettre la lecture de la même ressource pendant cette édition, ce qui peux causer des problèmes comme des valeurs incorrectes quand un programme lis un fichier qui n'a pas encore été éditer par le service apportant les données en temps réel (exemple: Google Map lis le fichier de position GPS avant que le service de positionnement GPS n'inscrive la position actuelle du téléphone dans ce fichier. Donc au lieu d'être à Nantes tu es encore à Marseille pour Google Map).

Certains système d'exclusion mutuelle choisissent, afin d'éviter cela, de ne pas permettre l'écriture d'une ressource pendant qu'elle est en train d'être lue; c'est notamment ce qui arrivée en 1997 durant la mission Mars Pathfinder quand les ingénieurs terrestres ont découvert que les données collectées par le robot Pathfinder étaient systématiquement effacée car un processus avait bloquer la mémoire partagée utilisée pour stocker les données collectées en mode écriture, empéchant le processus du disque dur de lire cette mémoire pour enregistrer les données..



### Solutions possibles

Biensûr, ce genre de cas de figure avait déjà été imaginée ou rencontré et des contre-mesures avaient été créées; par exemple: Certains programmeurs choisissent d'imposer l'ordre d'acquisition des ressources pour éviter qu'un processus de faible priorité (exemple: le widget météo) bloque un processus plus important (exemple: le clavier) ou encore d'implémanter des systèmes de récupération au cas où leur premier garde-fou échoue. 
Pour revenir sur l'histoire de Pathfinder: La NASA avait inclu les deux, un redémarrage automatique du système s'effectuait donc quand un processus occupait une ressource importante pendant trop longtemps, car ils assumaient que ce processus serait alors buggué. Donc Pathfinder redémarrait.. Encore.. et Encore.. Chaque fois en vidant la RAM contenant les données temporaires... Oups. Heureusement, les ingénieurs s'en sont rendus compte et ont corrigés le code qui empêchait la libération de la ressource système 

(Le problème d'un robot d'exploration c'est que le logiciel gérant les sondes atmosphériques, thermiques, etc est un de ceux qui démarre en premier. Donc Pathfinder retournait constamment à son état bloqué car ce même logiciel ne relâchait pas la mémoire partagée).



#### Précision

Pathfinder n'utilisait pas l'algorithme Ricart-Agrawala, c'était uniquement un exemple pour l'exclusion mutuelle.






## Système distribué

Un système distribué, contrairement à un système centralisé, est un système dont les ressources ne sont pas au même endroit. Exemple: Internet. Vous n'avez pas toutes les données d'internet sur votre disque dur.

Un système centralisé serait donc un ordinateur sans aucun réseau autre que celui le liant à ses composants. 






## A quoi sert l'algorithme Ricart-Agrawala ?

L'algorithme de Glenn Ricart et Ashok K. Agrawala est une optimisation de l'algorithme d'exclusion mutuelle de Lamport.



### Algorithme d'exclusion mutuelle de Lamport

L'algorithme d'exclusion mutuelle de Lamport fonctionne, de manière très grossière, comme ceci:
- Chaque processus à une pile de demandes d'accès à des ressources mutuellement excluse (ressources que l'on ne peux pas lire et écrire ou écrire avec 2 processus à la fois).

- Pour demander l'accès à une ressource, un processus envoie une demande d'accès dans sa propre pile de demandes d'accès afin d'y inscrire le moment de la demande en "Temps Lamport" (Une horloge logique (pas physique mais logicielle) qui sert à synchroniser des processus asynchrones entre eux. Quand un processus reçoit une requête, il se connecte brièvement au processus à l'origine de la requête et synchronise son horaire avec lui.
A noter que si deux événements (genre écrire "Chat" dans "Chat.txt" et "Chien" dans "Chien.txt") ont lieu sans interagir l'un avec l'autre, l'horloge de Lamport sera incapable de savoir lequel à eu lieu en premier; elle ne sert qu'a synchroniser des processus interagissant entre eux.)

- Il envoie sa demande d'accès à tous les services et attends la réponse de tous les services.

- Il accède à la ressource **une fois que sa requête est en haut de la pile** (Car les requêtes d'accès des autres services/processus (synonyme) sont aussi stockés dans cette pile) **et qu'il à reçu la réponse de tout les processus**.

- Une fois son accès à la ressource terminée, il retire sa demande d'accès de sa propre pile et envoie un message indiquant aux autres services que son accès est terminé et que la ressource est de nouveau libre. Les autres services retirent donc également la demande d'accès du premier service à cette ressource de leurs piles.



### Comment l'algorithme Ricart-Agrawala  optimise-t-il l'algorithme de Lamport ?

L'algorithme utilise l'horloge de Lamport (Un autre algorithme utilisé pour informer les processus d'un système asynchrone des relations de causalité de leurs événements. En gros: Dire à plusieurs processus ne s'exécutant pas en même temps "Qu'est-ce qui est arrivé en premier, et plus généralement qu'est-ce qui est arrivé avant moi ?". **Sauf que lui le fait "tout seul" sans broadcast chaque accès et libération de ressource à tous les services**.) afin de diminuer le nombre de messages échangés par accès à des ressources mutuellement exclusives et à complètement éliminer le besoin de messages de libérations.

Fonctionnement (Presque le même principe que l'algorithme d'exclusion mutuelle de Lamport):

- Quand un service veux accéder à une ressource, il envoie un message à tous les autres processus afin de leur demander si ils utilisent la ressource.

- Si tous les processus lui répondent qu'ils n'utilisent pas actuellement la ressource, alors il accède à la ressource. 
Si notre service ne reçoit pas une réponse de la part d'un des processus, il assume que ce processus utilise actuellement la ressource cible et reste en attente. (Le processus utilisant la ressource répondra qu'il n'utilise plus la ressource après qu'il a libéré la ressource **il ne dira jamais explicitement qu'il est en train de l'utiliser, c'est là qu'on gagne des messages**)

- Une fois l'accès à la ressource obtenue, il commence à l'écrire/la lire.

- Le service *n'enverra pas forcément* de message de libération de la ressource. Il ne le fera **que si un autre processus avait demander si la ressource été libre pendant qu'il l'occuppait**. Donc si personne ne voulait la ressource pendant qu'il l'éditait: Il n'avertira personne que la ressource est libre. On économise encore plus de messages !


Cela fonctionne car les processus ne se 'souviennent pas' de quelle ressources sont en train d'être utilisée par d'autres services; ils ne connaissent que les ressources qu'ils utilisent et assument que toutes les autres ressources sont libres jusqu'à ce qu'ils ne reçoivent pas de réponse quand ils demandant si cette ressource est occupée. 
Ils n'ont donc pas besoin de recevoir de message de libération parlant d'une ressource dont ils n'avaient rien à faire.

Problème possible: Si un processus se bloque sur une ressource, il ne répondra jamais aux messages des autres et le bloquera 'pour toujours'. Oui. Mais comme pour le robot Pathfinder, les programmeurs auront (probablement) mit en place un garde-fou qui gérera ce cas de figure, par exemple en redémarrant tout le système ou juste le service affecté si il cesse de répondre pendant 'trop longtemps' (valeure abritraire définie par le programmer du garde-fou).



### Détails

L'algorithme d'exclusion mutuelle de Ricart-Agrawala à une **complexité de 2 * (N - 1)**, où N est le nombre de processus tentant d'accèder à une ressource mutuellement exclusive. (Voir https://de.wikipedia.org/wiki/Ricart-Agrawala-Algorithmus (Version allemande de la page); les autres versions ne détaille pas le calcul de complexité (English) ou utilisent un mot qui apparaît exclusivement dans l'explication du calcul, sans explication de ce qu'il représente (Français))

L'algorithme d'exclusion mutuelle de Lamport, lui, avait **une complexité de 3 * (N -1)**; où N est également le nombre de processus tentant d'accèder à une ressource mutuellement exclusive. (Ce qui est très bien détaillé dans la version anglaise ( https://en.wikipedia.org/wiki/Lamport's_distributed_mutual_exclusion_algorithm ) de la page; heureusement, car c'est la seule version de la page qui existe!)
