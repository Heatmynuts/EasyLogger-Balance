# EasyLogger Balance

**Balance WiFi pour M5AtomS3** - Fonctionne avec le projet [StamPLC_Complete_SPIFFS](https://github.com/Heatmynuts/StamPLC_Complete_SPIFFS)

## Description

EasyLogger Balance est un firmware pour M5AtomS3 qui permet de connecter une balance série (A&D ou Sartorius) et de transmettre les pesées en temps réel via WiFi/WebSocket.

## Caractéristiques

- 📡 **Connexion WiFi** - Se connecte automatiquement au réseau du StamPLC
- ⚖️ **Support multi-balances** - Compatible A&D et Sartorius
- 🔄 **Temps réel** - Transmission WebSocket à 20 pesées/seconde
- 🖥️ **Interface Web** - Configuration et monitoring intégrés
- 🎛️ **Mode StamPLC** - Format de données normalisé pour EasyDose

## Compatibilité

| Balance | Baud | Config | Status |
|---------|------|--------|--------|
| A&D | 2400 | 7E1 | ✅ Testé |
| Sartorius BCE | 9600 | 8O1 | ✅ Testé |

## Installation

1. Ouvrir `ATOMS3-GCA.ino` dans Arduino IDE
2. Sélectionner la carte **M5AtomS3**
3. Téléverser le code
4. Se connecter au réseau WiFi `Easylogger-XXXX` (mot de passe: `easylogger`)
5. Accéder à l'interface web pour configurer

## Configuration avec StamPLC

1. Dans les **Réglages** de l'AtomS3, activer "Connexion automatique au réseau StamPLC"
2. Entrer le SSID et mot de passe du réseau WiFi
3. Activer le **Mode StamPLC** pour le format de données normalisé
4. Dans EasyDose (StamPLC), configurer l'adresse IP de l'AtomS3

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

## Voir aussi

- [StamPLC_Complete_SPIFFS](https://github.com/Heatmynuts/StamPLC_Complete_SPIFFS) - Contrôleur de dosage EasyDose

