# Intro (L'humour de 16 tonnes 5 disparaît après 'Actual Technical Stuff')

## Le PB:
Les réseaux sont foireux par nature (trop de parties indépendantes) et donc on perd souvent des paquets car ils sont trop gros pour passer par l'intermédiaire n°500 013

## La solution:
Inventer le TCP qui est un protocole s'attendant à recevoir une confirmation de réception pour chaque paquet envoyé et qui renvoie la donnée si il ne reçoit pas de confirmation au bout d'un certain délai. Super, ça fonctionne, on ne perds plus de paquet et internet est désormais aussi robuste que nécessaire pour communiquer depuis n'importe où!

non.

## Le nouveau problème:
Quand un paquet est trop gros pour l'intermédiaire n°500 013, l'intermédiaire ne le traite pas et n'informe personne du problème, sachant que de toute façon, le paquet sera renvoyer par l'origine. Donc TCP continue d'envoyer deux nouveaux paquets, attend les réponses.. Qui ne viennent pas et envoie donc de nouveau les trois nouveaux paquets.. Qui bloquent à nouveau... Ouais... Ils vont probablement continuer à faire ça longtemps.


## La nouvelle solution:
Faire des tests avant d'envoyer les données: on va envoyer des paquets bidons de plus en plus gros jusqu'à ce qu'un ne soit pas reçu, après quoi on va retourner à la dernière taille recevable et l'augmenter octet par octet jusqu'à ce qu'il ne soit de nouveau plus recevable. On saura donc la taille maximale que l'on peut transmettre pour rester efficace. Yay !

C'est la solution cette fois, pas vrai ?
no- okay si, celle la fonctionne et c'est pourquoi on va en parler.




# Actual technical stuff


## TCP - Transmission Control Protocol
Le TCP ou Transmission Control Protocol, développé par Vint Cert et Bob Kahn puis introduit à l'internet en 1974 est un des protocoles les plus utilisés d'internet d'après Wikipédia.

Le protocole TCP établit une connexion entre l'envoyeur des informations et leur receveur, une fois la connexion confirmée par l'envoi d'un rapide message de test, on commence à envoyer les données en sachant que chaque segment TCP (le truc qui contient les données) reçu causera l'envoie d'une confirmation par son receveur. 

Puisqu'être rapide c'est important, on en envoie plusieurs d'un coup et on attends les confirmations.
Enfin, en assumant que l'on soit encore avant 1986 quand le problème potentiel avait été identifié et ignoré jusqu'à ce qu'un des réseaux principal de la NSFNET, la fondation des réseaux scientifiques des Etats Unis rencontre un problème et passe d'un très respectable taux de transfert de 32 kilobits par secondes à 40 bits par secondes; un problème qui est nommé:


## Congestion Collapse (L'effondrement de la congestion)
Puisque le protocole TCP renvois les segments pour lesquels il n'a pas reçu de confirmation de réception, et qu'il en envoie __plusieurs à la fois__, quand un réseau tombe à 40 bits par secondes au lieu de 32 kilobits par secondes, il continue pendant un moment d'envoyer des paquets de 32 kilobits qui sont donc silencieusement rejetés par l'intermédiaire NSFNET; sauf si ils ont la chance de faire moins que 40 bits.




## Contrôle de congestion

il va falloir commencer calculer combien d'informations je peux envoyer sur la route en même temps sans que la route ne s'effondre.


## AIMD - Additive Increase/Multiplicative Decrease algorithm
L'algorithme d'augmentation additive et de réduction multiplicative est un algorithme que l'on qualifiera de closed loop control algorithm.

Basiquement, TCP va utiliser AIMD pour envoyer des paquets de plus en plus gros les uns à la suite des autres jusqu'à ne plus avoir de réponse (en faisant des additions à la taille, d'où le additive de AIMD), après quoi on réduit multiplicativement la taille. Ensuite on recommence jusqu'à ne plus pouvoir additionner sans perdre le paquet! 
Vous pouvez imaginer que l'on ajoute 5 octets au paquet de test chaque fois qu'il est reçu et qu'on divise ses octets par deux quand ce n'est pas le cas.


## CWND - Congestion WiNDow
Cela nous permet d'établir la CWND ou fenêtre de congestion. Contrairement à la receive window qui est maintenue et communiquée par le receveur des informations (rappel: la fenêtre qui dit combiens d'octets l'envoyeur peux balancer d'un coup sans attendre de réponse, on l'a aussi vue sous le nom de sliding window. Elle utilise elle aussi AIMD), la CWND elle est maintenue et gardée par l'envoyeur des données à transmettre.

Elle n'est pas communiquée car elle ne sert qu'à savoir quels taille de paquets sont recevables par le receveur car bien qu'il soit utile de communiquer la receive window à l'origine des infos puisqu'elle est calculée par la destination mais utilisée par l'origine, et la sliding window à la destination car elle est calculée par l'origine mais utilisée par la destination; envoyée la congestion window est inutile et n'est donc pas fait.
