## Intro


# Le PB:
Les réseaux sont foireux par nature (trop de parties indépendantes) et donc on perd souvent des paquets car ils sont trop gros pour passer par l'intermédiaire n°500 013

# La solution:
Inventer le TCP qui est un protocole s'attendant à recevoir une confirmation de réception pour chaque paquet envoyé et qui renvoie la donnée si il ne reçoit pas de confirmation au bout d'un certain délai. Super, ça fonctionne, on ne perds plus de paquet et internet est désormais aussi robuste que nécessaire pour communiquer depuis n'importe où!

non.

# Le nouveau problème:
Quand un paquet est trop gros pour l'intermédiaire n°500 013, l'intermédiaire ne le traite pas et n'informe personne du problème, sachant que de toute façon, le paquet sera renvoyer par l'origine. Donc TCP continue d'envoyer deux nouveaux paquets, attend les réponses.. Qui ne viennent pas et envoie donc de nouveau les trois nouveaux paquets.. Qui bloquent à nouveau... Ouais... Ils vont probablement continuer à faire ça longtemps.


# La nouvelle solution:
Faire des tests avant d'envoyer les données: on va envoyer des paquets bidons de plus en plus gros jusqu'à ce qu'un ne soit pas reçu, après quoi on va retourner à la dernière taille recevable et l'augmenter octet par octet jusqu'à ce qu'il ne soit de nouveau plus recevable. On saura donc la taille maximale que l'on peut transmettre pour rester efficace. Yay !

C'est la solution cette fois, pas vrai ?
no- okay si, celle la fonctionne et c'est pourquoi on va en parler.




## Actual technical stuff


# Thé C'est Pet toi-même.
Le TCP ou Transmission Control Protocol, développé par Vint Cert et Bob Kahn puis introduit à l'internet en 1974 est un des protocoles les plus utilisés d'internet.. D'après wikipédia, j'espère que vous n'attendez pas de preuve car je ne fais pas mon alternance à l'Internet Engineering Task Force (ce serait probablement vachement intéréssant). Petit rappel très grossier de comment ça fonctionne: Le protocole TCP établit une connexion entre l'envoyeur des informations et leur receveur, une fois la connexion confirmée par l'envoi d'un rapide message de test, on commence à envoyer les données en sachant que chaque segment TCP (le truc qui contient les données) reçu causera l'envoie d'une confirmation par son receveur. Puisqu'être rapide c'est important, on en envoie plusieurs d'un coup et on attends les confirmations... 
Enfin, en assumant que l'on soit encore avant 1986 quand le problème potentiel avait été identifié et ignoré jusqu'à ce qu'un des réseaux principal de la NSFNET, la fondation des réseaux scientifiques des Etats Unis rencontre un problème et passe d'un très respectable taux de transfert de 32 kilobits par secondes à..
Erm...
... 40.. bits.. par secondes... Aaaah.. Ca va.. causer un problème...
Le nom de ce problème?


# L'effondrement de la congestion
Ou l'effondrement de la route encombrée pour ceux qui comme moi, n'avait aucune idée de l'existence du mot congestion.
Comme je l'avais rapidement expliquer, quand le protocole TCP, ou protocole Transmission Control Protocol (oui soudainement j'ai l'air moins intelligent en disant ça, pensez-y la prochaine fois que vous direz protocole TCP)
Bref, on se souviens que quand le TCP ne reçoit pas de confirmation de réception de ses segments il les renvois, et il en renvois plusieurs à la suite car les envoyer un par un serait trop longs.
[Unoiled door hinge voice.midi]..Je vous laisse imaginer comment ça c'est passer avec un taux de transfert maximum tombé à 40 bits par seconde. Presque out le monde c'est retrouver bloquer sur la métaphorique autoroute de la recherche états-unienne et à dû refaire le trajet une bonne centaine de fois pour être le chanceux élu qui réussi à passer au milieu des paquets droppés.


#ENFIN! Le contrôle de congestion!
Donc, on a vu que tout renvoyer en s'armant de l'espoir que "cette fois se sera différent" ressemblait beaucoup à la folie qui je le rappel, est le principe de réessayer exactement la même chose en s'attendant à un résultat différent. Du coup il va falloir calculer combien d'informations je peux envoyer sur la route en même temps..

#AIMD
[digi_circus_cain_voicepack.mp3] Introducing... L'algorithme d'augmentation additive et de réduction multiplicative! Ou AIMD pour les intimes. Alors.. Non. Je n'expliquerais pas la formule en détail. Je suis devenu informaticien pour googler la plupart de mes problèmes tout en étant payé, pas pour expliquer des formules ressemblant à des bébés poulpes (ok j'exagère un peu). AIMD est un algorithme que l'on qualifiera de closed loop control algorithm, ou algorithme de contrôle à boucle fermée. Traduction pas transparente du tout, je sais!
Basiquement, TCP va utiliser AIMD pour envoyer des paquets de plus en plus gros les uns à la suite des autres jusqu'à ne plus avoir de réponse (en faisant des additions à la taille, d'où le additive de AIMD), après quoi on réduit.. Bravo, vous avez tous devinés: multiplicativement la taille. Ensuite on recommence jusqu'à ne plus pouvoir additionner sans perdre le paquet! Vous pouvez imaginer que l'on ajoute 5 octets au paquet de test chaque fois qu'il est reçu et qu'on divise ses octets par deux quand ce n'est pas le cas.

#CWND
Cela nous permet d'établir la CWND! Ou fenêtre de congestion (C pour Congestion, WND pour.. Window. On repassera pour l'équité.). Contrairement à la receive window qui est maintenue et communiquée par le receveur des informations (rappel: la fenêtre qui dit combiens d'octets l'envoyeur peux balancer d'un coup sans attendre de réponse, on l'a aussi vue sous le nom de sliding window. Elle utilise elle aussi AIMD), la CWND elle est maintenue et [goloum_voicepack.midi] jalousement gardée, la précieuse préciiieuse CWND.. par l'envoyeur. Elle est gardée par l'envoyeur et n'est pas communiquée car elle ne sert qu'à savoir quels taille de paquets sont recevables par.. Le receveur. Synonymes bonsoir! Nous avons des postes à pourvoir!
Bref. Autant c'est utile de communiquer la receive window à l'origine des infos puisqu'elle est calculée par la destination mais utilisée par l'origine, et la sliding window à la destination car elle est calculée par l'origine mais utilisée par la destination; envoyée la congestion window n'est pas utile et n'est donc pas fait.
