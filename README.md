# EasyLogger Balance

**Balance WiFi pour M5AtomS3**

## Description

EasyLogger Balance est un firmware pour M5AtomS3 qui permet de connecter une balance série (A&D ou Sartorius) et de transmettre les pesées en temps réel via WiFi/WebSocket.

## Caractéristiques

- 📡 **Connexion WiFi** - Mode client ou AP
- ⚖️ **Support multi-balances** - Compatible A&D et Sartorius
- 🔄 **Temps réel** - Transmission WebSocket à 20 pesées/seconde
- 🖥️ **Interface Web** - Configuration et monitoring intégrés

## Compatibilité

| Balance | Baud | Config | Status |
|---------|------|--------|--------|
| A&D | 2400 | 7E1 | ✅ Testé |
| Sartorius BCE | 9600 | 8O1 | ✅ Testé |

## Option DAC2 (Unit DAC2 GP8413)

Sortie analogique 0-10V proportionnelle à la pesée. Convertisseur 15 bits I2C avec étalonnage 6 points pour corriger les interférences.

**Bibliothèque requise :** DFRobot_GP8XXX (via Gestionnaire de bibliothèques Arduino)

## Installation

1. Ouvrir `ATOMS3-GCA.ino` dans Arduino IDE
2. Sélectionner la carte **M5AtomS3**
3. Téléverser le code
4. Se connecter au réseau WiFi `Easylogger-XXXX` (mot de passe: `easylogger`)
5. Accéder à l'interface web pour configurer

## API

| Endpoint | Méthode | Description |
|----------|---------|-------------|
| `/` | GET | Interface web de configuration |
| `/api/status` | GET | État actuel (poids, timestamp) |
| `/api/tare` | POST | Envoyer commande Tare |
| `/api/balance/command` | POST | Envoyer commande personnalisée |
| `/api/monitor` | GET | État écran + terminal |

## WebSocket (Port 81)

Les pesées sont diffusées en temps réel au format JSON :
```json
{"weight": "123.45 g"}
```

## Licence

MIT License - BDP France

