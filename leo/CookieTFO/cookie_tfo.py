import hashlib
import time
import random

SERVER_SECRET = "secret_serveur_tfo_2024"


def generer_cookie(client_ip: str) -> str:
    """Génère un cookie TFO basé sur l'IP client et un secret serveur."""
    data = f"{client_ip}{SERVER_SECRET}{int(time.time() // 3600)}"
    return hashlib.sha256(data.encode()).hexdigest()[:16]


def valider_cookie(client_ip: str, cookie: str) -> bool:
    """Valide un cookie TFO reçu du client."""
    expected = generer_cookie(client_ip)
    return cookie == expected


def simuler_premiere_connexion(client_ip: str) -> list[dict]:
    """
    Simule la première connexion TCP standard.
    Retourne les étapes sous forme de liste d'événements.
    """
    events = []

    events.append({
        "source": "CLIENT",
        "message": f"SYN → Connexion TCP standard vers le serveur",
        "detail": "Pas de cookie, pas de données applicatives",
        "type": "send"
    })

    events.append({
        "source": "SERVEUR",
        "message": f"SYN reçu de {client_ip}",
        "detail": "Handshake classique en cours...",
        "type": "receive"
    })

    cookie = generer_cookie(client_ip)

    events.append({
        "source": "SERVEUR",
        "message": f"Génération du cookie TFO",
        "detail": f"Cookie : {cookie}",
        "type": "process"
    })

    events.append({
        "source": "SERVEUR",
        "message": f"SYN-ACK → Cookie TFO inclus dans la réponse",
        "detail": f"Cookie transmis au client",
        "type": "send"
    })

    events.append({
        "source": "CLIENT",
        "message": "ACK → Connexion établie",
        "detail": f"Cookie stocké localement : {cookie}",
        "type": "receive"
    })

    return events, cookie


def simuler_connexion_future(client_ip: str, cookie: str, donnees: str) -> list[dict]:
    """
    Simule une connexion future avec cookie TFO + données dans le SYN.
    """
    events = []

    events.append({
        "source": "CLIENT",
        "message": "SYN + Cookie + Données → envoi en un seul paquet",
        "detail": f"Cookie: {cookie} | Données: {repr(donnees)}",
        "type": "send"
    })

    events.append({
        "source": "SERVEUR",
        "message": f"SYN reçu de {client_ip} avec cookie TFO",
        "detail": "Validation du cookie en cours...",
        "type": "receive"
    })

    valide = valider_cookie(client_ip, cookie)

    if valide:
        events.append({
            "source": "SERVEUR",
            "message": "Cookie valide — Traitement immédiat des données",
            "detail": f"Données traitées sans attendre le ACK final",
            "type": "success"
        })

        events.append({
            "source": "SERVEUR",
            "message": "SYN-ACK + Réponse applicative envoyés simultanément",
            "detail": "Un RTT économisé grâce au TFO !",
            "type": "send"
        })

        events.append({
            "source": "CLIENT",
            "message": "Réponse reçue — Connexion TFO réussie",
            "detail": "Latence réduite d'un aller-retour complet",
            "type": "success"
        })
    else:
        events.append({
            "source": "SERVEUR",
            "message": "Cookie invalide — Fallback vers handshake classique",
            "detail": "La connexion continue sans optimisation TFO",
            "type": "error"
        })

    return events, valide


def simuler_cookie_invalide(client_ip: str, donnees: str) -> list[dict]:
    """Simule une connexion avec un cookie invalide/expiré."""
    fake_cookie = "deadbeef12345678"
    return simuler_connexion_future(client_ip, fake_cookie, donnees)
