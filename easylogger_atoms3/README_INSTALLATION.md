# 📋 INSTALLATION StamPLC Complete avec SPIFFS

## 📁 Structure des fichiers

```
StamPLC_Complete_SPIFFS/
├── StamPLC_Complete_SPIFFS.ino  ← Code Arduino principal
└── data/
    └── index.html               ← Interface web (à uploader dans SPIFFS)
```

## 🛠️ Étape 1 : Installer le plugin SPIFFS Upload

### Pour Arduino IDE 1.x :
1. Téléchargez le plugin depuis : https://github.com/me-no-dev/arduino-esp32fs-plugin
2. Extrayez le fichier ZIP
3. Copiez le dossier `ESP32FS` dans `~/Documents/Arduino/tools/`
4. Redémarrez Arduino IDE

### Pour Arduino IDE 2.x :
1. Ouvrez le gestionnaire de bibliothèques
2. Recherchez "arduino-esp32fs-plugin"
3. Installez le plugin

## 📤 Étape 2 : Uploader le système de fichiers

1. **Créez le dossier `data`** dans le même dossier que votre fichier `.ino`
2. **Copiez `index.html`** dans le dossier `data/`
3. **Connectez votre M5Stack StamPLC** via USB
4. Dans Arduino IDE, allez dans **Outils → ESP32 Sketch Data Upload**
5. Attendez la fin de l'upload (environ 30 secondes)

⚠️ **IMPORTANT** : L'upload SPIFFS efface le programme actuel. Vous devrez téléverser le sketch après.

## 💾 Étape 3 : Compiler et téléverser le sketch

1. Ouvrez `StamPLC_Complete_SPIFFS.ino`
2. **Configurez votre WiFi** :
   ```cpp
   const char *WIFI_SSID     = "VotreSSID";
   const char *WIFI_PASSWORD = "VotreMotDePasse";
   ```
3. Sélectionnez **Board : "M5StamPLC"**
4. Vérifiez les **Partition Scheme** :
   - Dans Outils → Partition Scheme
   - Sélectionnez : **"Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"**
5. Cliquez sur **Téléverser**

## 🌐 Étape 4 : Accéder à l'interface

1. Ouvrez le **Moniteur Série** (115200 baud)
2. Notez l'**adresse IP** affichée (ex: `192.168.1.45`)
3. Ouvrez un navigateur et accédez à : `http://192.168.1.45`

## ✅ Vérification

Si tout fonctionne correctement, vous devriez voir :
- ✅ Dans le Serial Monitor : `[SPIFFS] OK`
- ✅ Dans le Serial Monitor : `[HTTP] Server OK on http://...`
- ✅ Dans le navigateur : Interface web complète

## 🔧 Dépannage

### Erreur "SPIFFS Mount Failed"
→ Le système de fichiers n'a pas été uploadé correctement
→ Refaites l'étape 2 (ESP32 Sketch Data Upload)

### Page web ne charge pas
→ Vérifiez que `index.html` est bien dans `data/`
→ Vérifiez le partition scheme (doit inclure SPIFFS)

### Erreur de mémoire
→ Sélectionnez un partition scheme avec plus d'espace SPIFFS
→ "Minimal SPIFFS (Large APPS with OTA)" peut aussi fonctionner

## 📊 Taille des fichiers

- **Code Arduino** : ~150 KB (au lieu de 1.3 MB !)
- **Fichier HTML** : ~25 KB
- **Total flash** : ~175 KB au lieu de >1.3 MB

## 🎉 Avantages de cette méthode

✅ **Code beaucoup plus petit** : Passe de 101% à ~12% d'utilisation
✅ **HTML modifiable** : Sans recompiler le code Arduino
✅ **Performances** : Lecture depuis SPIFFS très rapide
✅ **Séparation** : Code et interface web séparés

## 📝 Notes

- Vous pouvez éditer `index.html` et le ré-uploader sans toucher au code Arduino
- Pour mettre à jour l'interface : ESP32 Sketch Data Upload
- Pour mettre à jour le code : Upload normal

---

**Version** : 3.1 SPIFFS
**Date** : 21 Novembre 2025
