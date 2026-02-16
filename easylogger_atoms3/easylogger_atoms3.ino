/*
 * Easylogger M5AtomS3
 * BDP France - Version 2.1
 *
 * Affichage local
 */

#include "M5AtomS3.h"
#include "driver/temperature_sensor.h"
#include <DFRobot_GP8XXX.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WebSockets2_Generic.h>
#include <WiFi.h>

using namespace websockets2_generic;
#include <Wire.h>
#include <time.h>

// -------- Capteurs Internes --------
temperature_sensor_handle_t temp_sensor = NULL;
bool tempSensorInit = false;

void initTempSensor() {
  temperature_sensor_config_t temp_sensor_config =
      TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
  temperature_sensor_install(&temp_sensor_config, &temp_sensor);
  temperature_sensor_enable(temp_sensor);
  tempSensorInit = true;
}

float getCoreTemp() {
  if (!tempSensorInit)
    initTempSensor();
  float tsens_out;
  temperature_sensor_get_celsius(temp_sensor, &tsens_out);
  return tsens_out;
}

// Batterie ESP32-S3 (M5AtomS3 Power_Class — M5Unified)
// Retourne 0-100 ou -1 si non disponible (ex. alimentation USB sans batterie)
int8_t getBatteryLevel() {
  return (int8_t)AtomS3.Power.getBatteryLevel();
}
bool isBatteryCharging() {
  // is_charging_t: 0=discharging, 1=charging, 2=unknown
  return (AtomS3.Power.isCharging() == 1);
}

// Estimation Charge CPU (basée sur le temps de boucle)
unsigned long loopCount = 0;
unsigned long lastLoopCheck = 0;
float cpuLoad = 0.0;
// Baseline: nb boucles/sec à vide (approx 150k-200k sur ESP32-S3 240MHz)
const float MAX_LOOPS_PER_SEC = 200000.0;

void updateCpuLoad() {
  loopCount++;
  if (millis() - lastLoopCheck >= 1000) {
    // Charge = 1.0 - (Actuel / Max)
    // C'est une estimation très approximative mais donne une idée de la
    // "saturation" de la loop
    float ratio = (float)loopCount / MAX_LOOPS_PER_SEC;
    if (ratio > 1.0)
      ratio = 1.0;
    cpuLoad = (1.0 - ratio) * 100.0;

    // Clamp min/max pour réalisme
    if (cpuLoad < 0)
      cpuLoad = 0;
    if (cpuLoad > 100)
      cpuLoad = 100;

    loopCount = 0;
    lastLoopCheck = millis();
  }
}

// -------- RS232 (base M5 RS232) --------
static const int PIN_RX = 5, PIN_TX = 6;
static const uint32_t LINE_TIMEOUT_MS = 300;

// Types de balances
enum class BalanceType { AandD = 0, Sartorius = 1 };

// -------- UI / NTP --------
static const uint32_t IDLE_REFRESH_MS = 1000;
const char *TZ_EUROPE_PARIS = "CET-1CEST,M3.5.0/2,M10.5.0/3";
#define COL_GRAY 0x7BEF
#define COL_STABLE 0x03C0    // Vert foncé (ST)
#define COL_UNSTABLE 0xFD20  // Orange foncé (US)

// -------- Buffers --------
char lineBuf[64];
size_t lineLen = 0;
uint32_t lastByteMs = 0;

// -------- Config persistée --------
struct AppConfig {
  // Wi-Fi
  bool client_enabled = false;
  char ssid[33] = "";
  char pass[65] = "";

  // IP
  bool ip_dhcp = true;
  char ip[16] = "";
  char mask[16] = "";
  char gw[16] = "";
  char dns[16] = "";

  // UI
  char title[17] = "Easylogger";
  char idleText[17] = "En attente";
  uint8_t fontMain = 1;
  uint32_t showWeightMs = 2000;
  bool showWifiIndicator = true;
  uint8_t dtFormat = 0;
  char unit[10] = "KG"; // Unité par défaut
  bool weightColorByStability = false;  // Vert si valeur identique pendant X s
  uint32_t weightStabilityMs = 2000;   // Durée (ms) sans changement → stable (vert)
  bool weightHideLeadingZeros = false;  // +00066,20g -> 66,20g
  bool weightShowPlusSign = true;       // Afficher + ou seulement -

  // Balance - Par défaut : A&D, 2400, 7E1, CRLF, contrôle flux aucun
  uint8_t balanceType = 0; // 0=A&D, 1=Sartorius
  uint32_t balanceBaud = 2400;
  bool swapRxTx = false;

  uint8_t dataBits = 7;        // 7 bits
  uint8_t parity = 1;          // 0=Aucune, 1=Paire (E), 2=Impaire (O)
  uint8_t stopBits = 1;        // 1 bit d'arrêt
  uint8_t handshake = 0;       // 0=Aucun, 1=RTS/CTS, 2=XON/XOFF
  char terminator[5] = "\r\n"; // CRLF

  // Sortie DAC2 (Unit DAC2 GP8413 - 15 bits, I2C 0x59)
  bool dac2_enabled = false;
  float dac2_range_min = 0.0f;    // Plage début (g)
  float dac2_range_max = 1000.0f; // Plage fin (g)
  float dac2_cal_weight[6] = {0,   200, 400,
                              600, 800, 1000}; // Points calibration (g)
  uint16_t dac2_cal_mv[6] = {0,    2000, 4000,
                             6000, 8000, 10000}; // mV correspondants (0-10000)
} cfg;

// DAC2 GP8413 (I2C 0x59, 15 bits, 0-10V) - Unit DAC2 sur bus I2C
#define DAC2_I2C_ADDR 0x59
DFRobot_GP8413 g_dac2(DAC2_I2C_ADDR);
bool dac2Ok = false;

Preferences prefs;
WebServer server(80);

// -------- WebSocket (WebSockets2_Generic, basé sur ArduinoWebsockets) --------
#define WS_MAX_CLIENTS 4
WebsocketsServer wsServer;
WebsocketsClient wsClients[WS_MAX_CLIENTS];
bool wsClientInUse[WS_MAX_CLIENTS] = {false};

static uint8_t wsConnectedCount() {
  uint8_t n = 0;
  for (int i = 0; i < WS_MAX_CLIENTS; i++)
    if (wsClientInUse[i] && wsClients[i].available())
      n++;
  return n;
}
static void wsBroadcastTXT(const char* txt) {
  for (int i = 0; i < WS_MAX_CLIENTS; i++)
    if (wsClientInUse[i] && wsClients[i].available())
      wsClients[i].send(txt);
}
static void wsSendTXT(uint8_t num, const char* txt) {
  if (num < WS_MAX_CLIENTS && wsClientInUse[num] && wsClients[num].available())
    wsClients[num].send(txt);
}
static void wsLoop();

DNSServer dnsServer;
const byte DNS_PORT = 53;
bool dnsRunning = false;

enum class UiMode {
  IdleClock,
  ShowWeight,
  ConfirmFactory,
  SplashStaInfo,
  InfoOverlay,
  InfoSecrets
};
UiMode uiMode = UiMode::IdleClock;
uint32_t modeEnteredAt = 0;
bool wifiOK = false, timeReady = false;

String lastWeight = "";
uint32_t lastWeightMs = 0;
String lastDisplayedValue = "";        // Toujours la valeur formatée (sans ST/US) pour affichage
String lastFormattedValue = "";        // Pour calcul stabilité (durée identique)
uint32_t lastFormattedValueSince = 0;  // millis() quand cette valeur est apparue
uint32_t lastScreenSendMs = 0;         // Throttle envoi écran WebSocket (20 Hz max)
uint16_t lastDacMv = 0; // Dernière valeur envoyée au DAC (mV), pour affichage web
static const uint32_t SCREEN_SEND_INTERVAL_MS = 50;  // 20 Hz max pour écran (20 pesées/s)

// Variables pour le monitoring (terminal + écran)
String currentScreenContent = "";   // Contenu actuel de l'écran
String currentScreenMode = "idle";  // Mode: idle, weight, etc.
String terminalLog = "";            // Log du terminal (RX/TX)
const int MAX_TERMINAL_LINES = 100; // Nombre max de lignes dans le terminal

String currentAPSSID;
String hostName;

// Bouton
bool btnPressed = false;
uint32_t btnPressStart = 0;
const uint32_t LONG_PRESS_MS = 8000; // 8 secondes pour reset usine
const uint32_t CONFIRM_WINDOW_MS = 6000;
uint32_t confirmStartMs = 0;

// Triple-clic
uint8_t clickCount = 0;
uint32_t firstClickAt = 0;
const uint32_t CLICK_WINDOW_MS = 500;
uint32_t infoExpiresAt = 0;

// ===== Utils =====
String macSuffix() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char b[9];
  snprintf(b, sizeof(b), "%02X%02X", mac[4], mac[5]);
  return String(b);
}

String macAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char b[18];
  snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  return String(b);
}

// ===== Paramètres Balance =====
uint32_t getBalanceBaud() { return cfg.balanceBaud; }

int getBalanceSerialConfig() {
  char parityChar;
  switch (cfg.parity) {
  case 1:
    parityChar = 'E';
    break;
  case 2:
    parityChar = 'O';
    break;
  default:
    parityChar = 'N';
    break;
  }

  uint8_t data = constrain(cfg.dataBits, 5, 8);
  uint8_t stop = (cfg.stopBits == 2) ? 2 : 1;

  if (parityChar == 'N') {
    if (data == 5 && stop == 1)
      return SERIAL_5N1;
    if (data == 6 && stop == 1)
      return SERIAL_6N1;
    if (data == 7 && stop == 1)
      return SERIAL_7N1;
    if (data == 8 && stop == 1)
      return SERIAL_8N1;
    if (data == 5 && stop == 2)
      return SERIAL_5N2;
    if (data == 6 && stop == 2)
      return SERIAL_6N2;
    if (data == 7 && stop == 2)
      return SERIAL_7N2;
    if (data == 8 && stop == 2)
      return SERIAL_8N2;
  } else if (parityChar == 'E') {
    if (data == 5 && stop == 1)
      return SERIAL_5E1;
    if (data == 6 && stop == 1)
      return SERIAL_6E1;
    if (data == 7 && stop == 1)
      return SERIAL_7E1;
    if (data == 8 && stop == 1)
      return SERIAL_8E1;
    if (data == 5 && stop == 2)
      return SERIAL_5E2;
    if (data == 6 && stop == 2)
      return SERIAL_6E2;
    if (data == 7 && stop == 2)
      return SERIAL_7E2;
    if (data == 8 && stop == 2)
      return SERIAL_8E2;
  } else if (parityChar == 'O') {
    if (data == 5 && stop == 1)
      return SERIAL_5O1;
    if (data == 6 && stop == 1)
      return SERIAL_6O1;
    if (data == 7 && stop == 1)
      return SERIAL_7O1;
    if (data == 8 && stop == 1)
      return SERIAL_8O1;
    if (data == 5 && stop == 2)
      return SERIAL_5O2;
    if (data == 6 && stop == 2)
      return SERIAL_6O2;
    if (data == 7 && stop == 2)
      return SERIAL_7O2;
    if (data == 8 && stop == 2)
      return SERIAL_8O2;
  }

  return SERIAL_8N1;
}

int getBalanceRxPin() { return cfg.swapRxTx ? PIN_TX : PIN_RX; }

int getBalanceTxPin() { return cfg.swapRxTx ? PIN_RX : PIN_TX; }

String getBalanceCommand() {
  String cmd;
  if (cfg.balanceType == (uint8_t)BalanceType::AandD) {
    cmd = "Q";
  } else {
    cmd = "P";
  }
  cmd += String(cfg.terminator);
  return cmd;
}

// Fonction pour ajouter une ligne au terminal (ultra-optimisée pour 20Hz)
void addTerminalLog(const String &direction, const String &data) {
  // Vérification rapide - si pas de clients, on sort immédiatement
  if (wsConnectedCount() == 0) {
    return;
  }

  // Envoyer directement via WebSocket sans stocker dans terminalLog
  // Cela évite les opérations String coûteuses
  char json[256];
  char escapedData[128];
  int j = 0;

  // Échapper les caractères spéciaux de manière optimisée (sans String)
  for (int i = 0; i < data.length() && j < sizeof(escapedData) - 1; i++) {
    char c = data.charAt(i);
    if (c == '\\') {
      if (j < sizeof(escapedData) - 2) {
        escapedData[j++] = '\\';
        escapedData[j++] = '\\';
      }
    } else if (c == '"') {
      if (j < sizeof(escapedData) - 2) {
        escapedData[j++] = '\\';
        escapedData[j++] = '"';
      }
    } else if (c == '\n') {
      if (j < sizeof(escapedData) - 2) {
        escapedData[j++] = '\\';
        escapedData[j++] = 'n';
      }
    } else if (c == '\r') {
      if (j < sizeof(escapedData) - 2) {
        escapedData[j++] = '\\';
        escapedData[j++] = 'r';
      }
    } else if (isprint(c)) {
      escapedData[j++] = c;
    }
  }
  escapedData[j] = '\0';

  // Timestamp simplifié (on peut l'omettre pour gagner du temps)
  snprintf(json, sizeof(json),
           "{\"type\":\"terminal\",\"direction\":\"%s\",\"data\":\"%s\"}",
           direction.c_str(), escapedData);
  wsBroadcastTXT(json);
}

// Retourne true si la valeur affichée est stable (identique depuis weightStabilityMs)
bool isWeightStable() {
  if (lastFormattedValue.length() == 0) return false;
  if (lastDisplayedValue != lastFormattedValue) return false;
  return (millis() - lastFormattedValueSince) >= cfg.weightStabilityMs;
}

// Fonction pour mettre à jour l'état de l'écran
void updateScreenState() {
  // Mettre à jour l'état de l'écran pour l'interface web
  if (uiMode == UiMode::IdleClock) {
    currentScreenMode = "idle";
    currentScreenContent = cfg.idleText;
  } else if (uiMode == UiMode::ShowWeight) {
    currentScreenMode = "weight";
    currentScreenContent = lastDisplayedValue;  // Toujours formatée, jamais ST/US
  } else {
    currentScreenMode = "other";
    currentScreenContent = "";
  }

  // Envoyer via WebSocket si connecté
  uint8_t clients = wsConnectedCount();
  if (clients > 0) {
    char json[280];
    String escapedContent = currentScreenContent;
    escapedContent.replace("\\", "\\\\");
    escapedContent.replace("\"", "\\\"");

    if (currentScreenMode == "weight" && cfg.weightColorByStability) {
      bool st = isWeightStable();
      snprintf(json, sizeof(json),
               "{\"type\":\"screen\",\"mode\":\"weight\",\"content\":\"%s\",\"stable\":%s}",
               escapedContent.c_str(), st ? "true" : "false");
    } else {
      snprintf(json, sizeof(json),
               "{\"type\":\"screen\",\"mode\":\"%s\",\"content\":\"%s\"}",
               currentScreenMode.c_str(), escapedContent.c_str());
    }
    wsBroadcastTXT(json);
  }
}

// Initialiser la balance Sartorius en mode envoi continu
void initSartoriusContinuous() {
  if (cfg.balanceType != (uint8_t)BalanceType::Sartorius) {
    return; // Pas une Sartorius
  }

  Serial.println("[Balance] Initialisation mode continu Sartorius BCE...");
  delay(1000); // Attendre que la balance soit prête après l'initialisation
               // Serial2

  // Pour les balances Sartorius BCE, plusieurs commandes possibles selon le
  // modèle :
  // - "CONT" : Active l'envoi continu (commande standard)
  // - "SI" : Stream Immediate (envoi continu immédiat)
  // - "S" : Stream (mode stream)
  // - "P" : Print (demande une pesée unique, pas continu)

  // Essayer d'abord "CONT" (commande standard pour envoi continu)
  String cmd = "CONT";
  cmd += String(cfg.terminator);
  addTerminalLog("TX", cmd);
  Serial2.write((const uint8_t *)cmd.c_str(), cmd.length());
  Serial.printf("[TX] Sartorius CONT: %s", cmd.c_str());
  delay(300);

  // Certains modèles BCE nécessitent aussi "SI" après "CONT"
  // Essayer "SI" pour forcer l'envoi continu immédiat
  cmd = "SI";
  cmd += String(cfg.terminator);
  addTerminalLog("TX", cmd);
  Serial2.write((const uint8_t *)cmd.c_str(), cmd.length());
  Serial.printf("[TX] Sartorius SI: %s", cmd.c_str());
  delay(200);

  Serial.println("\n[Balance] Commandes d'initialisation envoyées");
  Serial.println(
      "[Balance] La balance devrait maintenant envoyer des données en continu");
  Serial.println("[Balance] Si aucune donnée n'arrive, essayez les commandes "
                 "via /api/balance/command?cmd=CONT ou cmd=SI");
}

IPAddress ipFrom(const char *s) {
  IPAddress ip;
  ip.fromString((String)s);
  return ip;
}

void saveConfig() {
  prefs.begin("easylog");
  // Wi-Fi
  prefs.putBool("cli", cfg.client_enabled);
  prefs.putString("ssid", cfg.ssid);
  prefs.putString("pass", cfg.pass);
  // IP
  prefs.putBool("dhcp", cfg.ip_dhcp);
  prefs.putString("ip", cfg.ip);
  prefs.putString("mask", cfg.mask);
  prefs.putString("gw", cfg.gw);
  prefs.putString("dns", cfg.dns);
  // UI
  prefs.putString("title", cfg.title);
  prefs.putString("idle", cfg.idleText);
  prefs.putUChar("fmain", cfg.fontMain);
  prefs.putUInt("showms", cfg.showWeightMs);
  prefs.putBool("wifiind", cfg.showWifiIndicator);
  prefs.putUChar("dtfmt", cfg.dtFormat);
  prefs.putString("unit", cfg.unit);
  prefs.putBool("wcolorst", cfg.weightColorByStability);
  prefs.putUInt("wstabms", cfg.weightStabilityMs);
  prefs.putBool("whidez", cfg.weightHideLeadingZeros);
  prefs.putBool("wplus", cfg.weightShowPlusSign);
  // Balance
  prefs.putUChar("btype", cfg.balanceType);
  prefs.putUInt("bbaud", cfg.balanceBaud);
  prefs.putBool("swaprt", cfg.swapRxTx);
  prefs.putUChar("dbits", cfg.dataBits);
  prefs.putUChar("parity", cfg.parity);
  prefs.putUChar("stop", cfg.stopBits);
  prefs.putUChar("hshake", cfg.handshake);
  prefs.putString("term", cfg.terminator);
  // DAC2
  prefs.putBool("dac2en", cfg.dac2_enabled);
  prefs.putFloat("dac2min", cfg.dac2_range_min);
  prefs.putFloat("dac2max", cfg.dac2_range_max);
  for (int i = 0; i < 6; i++) {
    char kw[8], km[8];
    snprintf(kw, sizeof(kw), "d2w%d", i);
    snprintf(km, sizeof(km), "d2m%d", i);
    prefs.putFloat(kw, cfg.dac2_cal_weight[i]);
    prefs.putUShort(km, cfg.dac2_cal_mv[i]);
  }
  prefs.end();
}

void loadConfig() {
  prefs.begin("easylog");
  // Wi-Fi
  cfg.client_enabled = prefs.getBool("cli", false);
  String s = prefs.getString("ssid", "");
  s.toCharArray(cfg.ssid, sizeof(cfg.ssid));
  s = prefs.getString("pass", "");
  s.toCharArray(cfg.pass, sizeof(cfg.pass));
  // IP
  cfg.ip_dhcp = prefs.getBool("dhcp", true);
  prefs.getString("ip", "").toCharArray(cfg.ip, sizeof(cfg.ip));
  prefs.getString("mask", "").toCharArray(cfg.mask, sizeof(cfg.mask));
  prefs.getString("gw", "").toCharArray(cfg.gw, sizeof(cfg.gw));
  prefs.getString("dns", "").toCharArray(cfg.dns, sizeof(cfg.dns));
  // UI
  s = prefs.getString("title", "Easylogger");
  s.toCharArray(cfg.title, sizeof(cfg.title));
  s = prefs.getString("idle", "En attente");
  s.toCharArray(cfg.idleText, sizeof(cfg.idleText));
  cfg.fontMain = prefs.getUChar("fmain", 1);
  cfg.showWeightMs = prefs.getUInt("showms", 2000);
  cfg.showWifiIndicator = prefs.getBool("wifiind", true);
  int legacy = prefs.getBool("dtdate", true) ? 0 : 2;
  cfg.dtFormat = prefs.getUChar("dtfmt", (uint8_t)legacy);
  s = prefs.getString("unit", "KG");
  s.toCharArray(cfg.unit, sizeof(cfg.unit));
  cfg.weightColorByStability = prefs.getBool("wcolorst", false);
  cfg.weightStabilityMs = prefs.getUInt("wstabms", 2000);
  cfg.weightHideLeadingZeros = prefs.getBool("whidez", false);
  cfg.weightShowPlusSign = prefs.getBool("wplus", true);
  // Balance
  cfg.balanceType = prefs.getUChar("btype", 0);

  // Si c'est une Sartorius, appliquer les paramètres optimaux par défaut
  if (cfg.balanceType == (uint8_t)BalanceType::Sartorius) {
    cfg.balanceBaud = 9600;
    cfg.dataBits = 8;
    cfg.parity = 2; // Impaire (O)
    cfg.stopBits = 1;
    cfg.handshake = 0;              // Aucun
    strcpy(cfg.terminator, "\r\n"); // CRLF
    cfg.swapRxTx = false;
    Serial.println(
        "[Config] Balance Sartorius détectée - Paramètres optimaux appliqués");
  } else {
    // Sinon, charger depuis la préférence
    cfg.balanceBaud = prefs.getUInt("bbaud", 2400);
    cfg.swapRxTx = prefs.getBool("swaprt", false);
    cfg.dataBits = prefs.getUChar("dbits", 7);
    cfg.parity = prefs.getUChar("parity", 1);
    cfg.stopBits = prefs.getUChar("stop", 1);
    cfg.handshake = prefs.getUChar("hshake", 0);
    s = prefs.getString("term", "\r\n");
    s.toCharArray(cfg.terminator, sizeof(cfg.terminator));
  }
  // DAC2
  cfg.dac2_enabled = prefs.getBool("dac2en", false);
  cfg.dac2_range_min = prefs.getFloat("dac2min", 0.0f);
  cfg.dac2_range_max = prefs.getFloat("dac2max", 1000.0f);
  for (int i = 0; i < 6; i++) {
    char kw[8], km[8];
    snprintf(kw, sizeof(kw), "d2w%d", i);
    snprintf(km, sizeof(km), "d2m%d", i);
    cfg.dac2_cal_weight[i] = prefs.getFloat(
        kw, cfg.dac2_range_min +
                (cfg.dac2_range_max - cfg.dac2_range_min) * i / 5.0f);
    cfg.dac2_cal_mv[i] =
        (uint16_t)prefs.getUShort(km, (uint16_t)(10000U * i / 5));
  }
  prefs.end();

  cfg.fontMain = constrain(cfg.fontMain, (uint8_t)1, (uint8_t)3);
  cfg.dtFormat = constrain(cfg.dtFormat, (uint8_t)0, (uint8_t)7);
  if (cfg.showWeightMs < 300)
    cfg.showWeightMs = 300;
  cfg.weightStabilityMs = constrain(cfg.weightStabilityMs, (uint32_t)500, (uint32_t)30000);
  cfg.balanceType = constrain(cfg.balanceType, (uint8_t)0, (uint8_t)1);
  if (cfg.balanceBaud < 600)
    cfg.balanceBaud = 2400;
  cfg.dataBits = constrain(cfg.dataBits, (uint8_t)5, (uint8_t)8);
  cfg.parity = constrain(cfg.parity, (uint8_t)0, (uint8_t)2);
  cfg.stopBits = (cfg.stopBits == 2) ? 2 : 1;
  cfg.handshake = constrain(cfg.handshake, (uint8_t)0, (uint8_t)2);
  if (strlen(cfg.terminator) == 0)
    strcpy(cfg.terminator, "\r\n");

  if (cfg.balanceType == (uint8_t)BalanceType::AandD &&
      cfg.balanceBaud == 2400) {
    // OK
  } else if (cfg.balanceType == (uint8_t)BalanceType::Sartorius &&
             cfg.balanceBaud == 2400) {
    cfg.balanceBaud = 9600;
  }
}

String nowDateTime() {
  if (!timeReady) {
    uint32_t s = millis() / 1000UL;
    char b[28];
    switch (cfg.dtFormat) {
    case 2:
    case 5:
    case 6:
      snprintf(b, sizeof(b), "%02lu:%02lu:%02lu",
               (unsigned long)((s / 3600UL) % 24UL),
               (unsigned long)((s / 60UL) % 60UL), (unsigned long)(s % 60UL));
      break;
    case 3:
    case 7:
      snprintf(b, sizeof(b), "%02lu:%02lu",
               (unsigned long)((s / 3600UL) % 24UL),
               (unsigned long)((s / 60UL) % 60UL));
      break;
    case 4:
      return "----/--/--";
    default:
      snprintf(b, sizeof(b), "----/--/-- %02lu:%02lu:%02lu",
               (unsigned long)((s / 3600UL) % 24UL),
               (unsigned long)((s / 60UL) % 60UL), (unsigned long)(s % 60UL));
      break;
    }
    return String(b);
  }
  time_t tnow = time(nullptr);
  if (tnow < 1700000000)
    return "--:--:--";
  struct tm info;
  localtime_r(&tnow, &info);
  char buf[28];
  switch (cfg.dtFormat) {
  case 1:
    strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &info);
    break;
  case 2:
    strftime(buf, sizeof(buf), "%H:%M:%S", &info);
    break;
  case 3:
    strftime(buf, sizeof(buf), "%H:%M", &info);
    break;
  case 4:
    strftime(buf, sizeof(buf), "%Y-%m-%d", &info);
    break;
  case 5:
    strftime(buf, sizeof(buf), "%H:%M:%S %Y-%m-%d", &info);
    break;
  case 6:
    strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", &info);
    break;
  case 7:
    strftime(buf, sizeof(buf), "%H:%M %Y-%m-%d", &info);
    break;
  default:
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &info);
    break;
  }
  return String(buf);
}

// ===== NOUVEAU: WebSocket Client =====

// ===== Wi-Fi / NTP =====
void startAP() {
  currentAPSSID = "Easylogger-" + macSuffix();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(currentAPSSID.c_str(), "easylogger");
  IPAddress apIP = WiFi.softAPIP();
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("[AP] SSID: %s  PASS: easylogger  IP: %s  MAC: %s\n",
                currentAPSSID.c_str(), apIP.toString().c_str(), macStr);
  dnsServer.start(DNS_PORT, "*", apIP);
  dnsRunning = true;
}

bool startClientWiFi() {
  if (!cfg.client_enabled || strlen(cfg.ssid) == 0) {
    Serial.println(
        "[WiFi] Pas de configuration WiFi client - démarrage en mode AP");
    return false;
  }

  const char *ssidToUse = cfg.ssid;
  const char *passToUse = cfg.pass;
  Serial.println("[WiFi] Note: Si la connexion échoue, l'AtomS3 démarrera en "
                 "mode AP (réseau Easylogger-XXXX)");

  WiFi.mode(WIFI_STA);

  hostName = "easylogger-" + macSuffix();
  WiFi.setHostname(hostName.c_str());

  if (!cfg.ip_dhcp) {
    IPAddress ip = ipFrom(cfg.ip);
    IPAddress mask = ipFrom(cfg.mask);
    IPAddress gw = ipFrom(cfg.gw);
    IPAddress dns1 = ipFrom(cfg.dns);
    if (ip && mask && gw) {
      WiFi.config(ip, gw, mask, dns1);
      Serial.printf("[IP] Statique %s / %s / %s  DNS:%s\n",
                    ip.toString().c_str(), mask.toString().c_str(),
                    gw.toString().c_str(), dns1.toString().c_str());
    } else {
      Serial.println("[IP] Paramètres statiques invalides, bascule en DHCP");
      cfg.ip_dhcp = true;
    }
  } else {
    Serial.println("[IP] DHCP");
  }

  WiFi.begin(ssidToUse, passToUse);
  Serial.printf("[Client Wi-Fi] Connexion SSID='%s'...\n", ssidToUse);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000)
    delay(100);
  wifiOK = (WiFi.status() == WL_CONNECTED);
  if (wifiOK) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf(
        "[Client Wi-Fi] OK. IP: %s  RSSI: %d dBm  MAC: %s  Host: %s.local\n",
        WiFi.localIP().toString().c_str(), WiFi.RSSI(), macStr,
        hostName.c_str());

    if (MDNS.begin(hostName.c_str())) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[mDNS] http://%s.local\n", hostName.c_str());
    } else {
      Serial.println("[mDNS] Echec MDNS.begin()");
    }

    setenv("TZ", TZ_EUROPE_PARIS, 1);
    tzset();
    configTzTime(TZ_EUROPE_PARIS, "pool.ntp.org", "time.google.com");
    for (int i = 0; i < 50; ++i) {
      if (time(nullptr) > 1700000000) {
        timeReady = true;
        break;
      }
      delay(100);
    }
    Serial.printf("[NTP] timeReady=%s\n", timeReady ? "true" : "false");

    uiMode = UiMode::SplashStaInfo;
    modeEnteredAt = millis();
    return true;
  }
  Serial.println("[Client Wi-Fi] Echec connexion.");
  return false;
}

// ===== Interface Web (ajout config Datalogger) =====
#include "page_index.h"
String htmlIndex() {
  auto ipcur = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
  String h = PAGE_INDEX;
  h.replace("%TITLE%", cfg.title);

  // Résumé réseau (sidebar Monitoring)
  {
    String networkSummary;
    if (WiFi.getMode() == WIFI_AP)
      networkSummary = "AP: " + currentAPSSID;
    else
      networkSummary = "STA: " + String(cfg.ssid);
    h.replace("%NETWORK_SUMMARY%", networkSummary);
  }

  // WiFi
  h.replace("%CLI_CHECK%", cfg.client_enabled ? "checked" : "");
  h.replace("%SSID%", cfg.ssid);
  h.replace("%PASS%", cfg.pass);
  // IP
  h.replace("%IP_DHCP_CHECK%", cfg.ip_dhcp ? "checked" : "");
  h.replace("%IP%", cfg.ip);
  h.replace("%MASK%", cfg.mask);
  h.replace("%GW%", cfg.gw);
  h.replace("%DNS%", cfg.dns);

  // UI
  h.replace("%IDLE%", cfg.idleText);
  h.replace("%FONT%", String(cfg.fontMain));
  h.replace("%SHOWMS%", String(cfg.showWeightMs));
  h.replace("%UNIT%", cfg.unit);
  h.replace("%DTFMT0%", (cfg.dtFormat == 0) ? "selected" : "");
  h.replace("%DTFMT1%", (cfg.dtFormat == 1) ? "selected" : "");
  h.replace("%DTFMT2%", (cfg.dtFormat == 2) ? "selected" : "");
  h.replace("%DTFMT3%", (cfg.dtFormat == 3) ? "selected" : "");
  h.replace("%DTFMT4%", (cfg.dtFormat == 4) ? "selected" : "");
  h.replace("%DTFMT5%", (cfg.dtFormat == 5) ? "selected" : "");
  h.replace("%DTFMT6%", (cfg.dtFormat == 6) ? "selected" : "");
  h.replace("%DTFMT7%", (cfg.dtFormat == 7) ? "selected" : "");
  h.replace("%WIFIIND_CHECK%", cfg.showWifiIndicator ? "checked" : "");
  h.replace("%WCOLORST_CHECK%", cfg.weightColorByStability ? "checked" : "");
  h.replace("%WSTABMS%", String(cfg.weightStabilityMs / 1000));
  h.replace("%WHIDEZ_CHECK%", cfg.weightHideLeadingZeros ? "checked" : "");
  h.replace("%WPLUS_CHECK%", cfg.weightShowPlusSign ? "checked" : "");

  // Balance
  h.replace("%BTYPE0%", (cfg.balanceType == 0) ? "selected" : "");
  h.replace("%BTYPE1%", (cfg.balanceType == 1) ? "selected" : "");
  h.replace("%BBAUD2400%", (cfg.balanceBaud == 2400) ? "selected" : "");
  h.replace("%BBAUD4800%", (cfg.balanceBaud == 4800) ? "selected" : "");
  h.replace("%BBAUD9600%", (cfg.balanceBaud == 9600) ? "selected" : "");
  h.replace("%BBAUD19200%", (cfg.balanceBaud == 19200) ? "selected" : "");

  h.replace("%DBITS5%", (cfg.dataBits == 5) ? "selected" : "");
  h.replace("%DBITS6%", (cfg.dataBits == 6) ? "selected" : "");
  h.replace("%DBITS7%", (cfg.dataBits == 7) ? "selected" : "");
  h.replace("%DBITS8%", (cfg.dataBits == 8) ? "selected" : "");

  h.replace("%PARITY0%", (cfg.parity == 0) ? "selected" : "");
  h.replace("%PARITY1%", (cfg.parity == 1) ? "selected" : "");
  h.replace("%PARITY2%", (cfg.parity == 2) ? "selected" : "");

  h.replace("%STOPBITS1%", (cfg.stopBits == 1) ? "selected" : "");
  h.replace("%STOPBITS2%", (cfg.stopBits == 2) ? "selected" : "");

  h.replace("%HANDSHAKE0%", (cfg.handshake == 0) ? "selected" : "");
  h.replace("%HANDSHAKE1%", (cfg.handshake == 1) ? "selected" : "");
  h.replace("%HANDSHAKE2%", (cfg.handshake == 2) ? "selected" : "");

  String term = String(cfg.terminator);
  h.replace("%TERM_CRLF%", (term == "\r\n") ? "selected" : "");
  h.replace("%TERM_CR%", (term == "\r") ? "selected" : "");
  h.replace("%TERM_LF%", (term == "\n") ? "selected" : "");
  h.replace("%TERM_NONE%", (term.length() == 0) ? "selected" : "");

  h.replace("%SWAPRT%", cfg.swapRxTx ? "checked" : "");

  // DAC2
  h.replace("%DAC2EN_CHECK%", cfg.dac2_enabled ? "checked" : "");
  h.replace("%DAC2MIN%", String(cfg.dac2_range_min, 1));
  h.replace("%DAC2MAX%", String(cfg.dac2_range_max, 1));
  h.replace("%DAC2W0%", String(cfg.dac2_cal_weight[0], 2));
  h.replace("%DAC2M0%", String(cfg.dac2_cal_mv[0]));
  h.replace("%DAC2W1%", String(cfg.dac2_cal_weight[1], 2));
  h.replace("%DAC2M1%", String(cfg.dac2_cal_mv[1]));
  h.replace("%DAC2W2%", String(cfg.dac2_cal_weight[2], 2));
  h.replace("%DAC2M2%", String(cfg.dac2_cal_mv[2]));
  h.replace("%DAC2W3%", String(cfg.dac2_cal_weight[3], 2));
  h.replace("%DAC2M3%", String(cfg.dac2_cal_mv[3]));
  h.replace("%DAC2W4%", String(cfg.dac2_cal_weight[4], 2));
  h.replace("%DAC2M4%", String(cfg.dac2_cal_mv[4]));
  h.replace("%DAC2W5%", String(cfg.dac2_cal_weight[5], 2));
  h.replace("%DAC2M5%", String(cfg.dac2_cal_mv[5]));

  return h;
}

// ===== HTTP Handlers =====
void sendCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
  IPAddress clientIP = server.client().remoteIP();
  Serial.printf("[HTTP] GET / - Client IP: %s\n", clientIP.toString().c_str());
  Serial.printf("[HTTP] WiFi status: %d (1=STA, 2=AP, 3=STA+AP)\n",
                WiFi.getMode());

  sendCORS();

  Serial.println("[HTTP] Generating HTML...");
  String html = htmlIndex();
  Serial.printf("[HTTP] HTML generated: %d bytes\n", html.length());

  if (html.length() == 0) {
    Serial.println("[HTTP] ERROR: HTML is empty!");
    server.send(500, "text/plain", "Error generating HTML");
    return;
  }

  Serial.println("[HTTP] Sending HTML...");
  server.send(200, "text/html", html);
  Serial.println("[HTTP] HTML sent successfully");
}

// Appliquer les réglages UI sans redémarrer
void applyUISettings() {
  // Redessiner l'écran avec les nouveaux paramètres
  if (uiMode == UiMode::IdleClock) {
    drawIdleScreen();
  } else if (uiMode == UiMode::ShowWeight) {
    drawWeightScreen(lastDisplayedValue);
  }

  // Mettre à jour l'état pour l'interface web
  updateScreenState();

  Serial.println("[Config] Paramètres UI appliqués sans redémarrage");
}

// Réinitialiser Serial2 avec les nouveaux paramètres
void reinitSerial2() {
  Serial.println("[Config] Réinitialisation de Serial2...");

  // Arrêter Serial2
  Serial2.end();
  delay(100);

  // Réinitialiser avec les nouveaux paramètres
  uint32_t baud = getBalanceBaud();
  int serialConfig = getBalanceSerialConfig();
  int rxPin = getBalanceRxPin();
  int txPin = getBalanceTxPin();

  Serial2.begin(baud, serialConfig, rxPin, txPin);

  char parityChar = (cfg.parity == 1) ? 'E' : ((cfg.parity == 2) ? 'O' : 'N');
  char configStr[8];
  snprintf(configStr, sizeof(configStr), "%d%c%d", cfg.dataBits, parityChar,
           cfg.stopBits);

  Serial.printf(
      "[Balance] Réinitialisé - Type: %s, Baud: %lu, Config: %s, RX: %d, TX: "
      "%d\n",
      (cfg.balanceType == (uint8_t)BalanceType::AandD ? "A&D" : "Sartorius"),
      baud, configStr, rxPin, txPin);

  // Si c'est une Sartorius, réinitialiser le mode continu
  if (cfg.balanceType == (uint8_t)BalanceType::Sartorius) {
    delay(1000);
    initSartoriusContinuous();
  }

  Serial.println("[Config] Serial2 réinitialisé");
}

void handleSave() {
  String s;

  // Sauvegarder les anciennes valeurs WiFi pour détecter les changements
  bool oldClientEnabled = cfg.client_enabled;
  String oldSSID = String(cfg.ssid);
  String oldPass = String(cfg.pass);

  // Sauvegarder les anciennes valeurs IP pour détecter les changements
  bool oldIpDhcp = cfg.ip_dhcp;
  String oldIp = String(cfg.ip);
  String oldMask = String(cfg.mask);
  String oldGw = String(cfg.gw);
  String oldDns = String(cfg.dns);

  // WiFi
  cfg.client_enabled = server.hasArg("cli");
  s = server.arg("ssid");
  s.toCharArray(cfg.ssid, sizeof(cfg.ssid));
  s = server.arg("pass");
  s.toCharArray(cfg.pass, sizeof(cfg.pass));

  // IP
  cfg.ip_dhcp = server.hasArg("ip_dhcp");
  server.arg("ip").toCharArray(cfg.ip, sizeof(cfg.ip));
  server.arg("mask").toCharArray(cfg.mask, sizeof(cfg.mask));
  server.arg("gw").toCharArray(cfg.gw, sizeof(cfg.gw));
  server.arg("dns").toCharArray(cfg.dns, sizeof(cfg.dns));

  // UI
  s = server.arg("title");
  s.toCharArray(cfg.title, sizeof(cfg.title));
  s = server.arg("idle");
  s.toCharArray(cfg.idleText, sizeof(cfg.idleText));
  cfg.fontMain = (uint8_t)constrain(server.arg("font").toInt(), 1, 3);
  cfg.showWeightMs =
      (uint32_t)constrain(server.arg("showms").toInt(), 300, 10000);
  s = server.arg("unit");
  s.toCharArray(cfg.unit, sizeof(cfg.unit));
  cfg.dtFormat = (uint8_t)constrain(server.arg("dtfmt").toInt(), 0, 7);
  cfg.showWifiIndicator = server.hasArg("wifiind");
  cfg.weightColorByStability = server.hasArg("wcolorst");
  int stabSec = constrain(server.arg("wstabms").toInt(), 1, 30);
  cfg.weightStabilityMs = (uint32_t)stabSec * 1000;
  cfg.weightHideLeadingZeros = server.hasArg("whidez");
  cfg.weightShowPlusSign = server.hasArg("wplus");

  // Sauvegarder les anciennes valeurs Balance pour détecter les changements
  uint8_t oldBalanceType = cfg.balanceType;
  uint32_t oldBalanceBaud = cfg.balanceBaud;
  uint8_t oldDataBits = cfg.dataBits;
  uint8_t oldParity = cfg.parity;
  uint8_t oldStopBits = cfg.stopBits;
  uint8_t oldHandshake = cfg.handshake;
  bool oldSwapRxTx = cfg.swapRxTx;
  String oldTerminator = String(cfg.terminator);

  // Balance
  uint8_t newBalanceType =
      (uint8_t)constrain(server.arg("btype").toInt(), 0, 1);

  // Si le type change vers Sartorius, appliquer automatiquement les paramètres
  // optimaux
  if (newBalanceType == (uint8_t)BalanceType::Sartorius &&
      oldBalanceType != (uint8_t)BalanceType::Sartorius) {
    Serial.println("[Config] Type changé vers Sartorius - Application des "
                   "paramètres optimaux");
    cfg.balanceType = newBalanceType;
    cfg.balanceBaud = 9600;
    cfg.dataBits = 8;
    cfg.parity = 2; // Impaire (O)
    cfg.stopBits = 1;
    cfg.handshake = 0;              // Aucun
    strcpy(cfg.terminator, "\r\n"); // CRLF
    cfg.swapRxTx = false;
  } else {
    // Sinon, utiliser les valeurs du formulaire
    cfg.balanceType = newBalanceType;
    int baud = server.arg("bbaud").toInt();
    if (baud == 2400 || baud == 4800 || baud == 9600 || baud == 19200) {
      cfg.balanceBaud = (uint32_t)baud;
    }

    cfg.dataBits = (uint8_t)constrain(server.arg("dbits").toInt(), 5, 8);
    cfg.parity = (uint8_t)constrain(server.arg("parity").toInt(), 0, 2);
    cfg.stopBits = (uint8_t)constrain(server.arg("stopbits").toInt(), 1, 2);
    cfg.handshake = (uint8_t)constrain(server.arg("handshake").toInt(), 0, 2);
    cfg.swapRxTx = server.hasArg("swaprt");
  }

  // DAC2
  cfg.dac2_enabled = server.hasArg("dac2en");
  cfg.dac2_range_min = server.arg("dac2min").toFloat();
  cfg.dac2_range_max = server.arg("dac2max").toFloat();
  for (int i = 0; i < 6; i++) {
    char nw[16], nm[16];
    snprintf(nw, sizeof(nw), "dac2w%d", i);
    snprintf(nm, sizeof(nm), "dac2m%d", i);
    cfg.dac2_cal_weight[i] = server.arg(nw).toFloat();
    cfg.dac2_cal_mv[i] = (uint16_t)constrain(server.arg(nm).toInt(), 0, 10000);
  }
  if (cfg.dac2_range_max <= cfg.dac2_range_min)
    cfg.dac2_range_max = cfg.dac2_range_min + 1.0f;

  // Terminator - seulement si on n'a pas déjà appliqué les paramètres Sartorius
  if (!(newBalanceType == (uint8_t)BalanceType::Sartorius &&
        oldBalanceType != (uint8_t)BalanceType::Sartorius)) {
    s = server.arg("terminator");
    // Convertir les codes en caractères réels
    if (s == "CRLF") {
      strcpy(cfg.terminator, "\r\n");
    } else if (s == "CR") {
      strcpy(cfg.terminator, "\r");
    } else if (s == "LF") {
      strcpy(cfg.terminator, "\n");
    } else if (s == "NONE" || s.length() == 0) {
      cfg.terminator[0] = '\0';
    } else {
      // Valeur personnalisée (fallback)
      s.toCharArray(cfg.terminator, sizeof(cfg.terminator));
    }
  }

  // Détecter si les paramètres WiFi / réseau ont changé (redémarrage requis)
  bool wifiChanged = (oldClientEnabled != cfg.client_enabled) ||
                     (oldSSID != String(cfg.ssid)) ||
                     (oldPass != String(cfg.pass)) ||
                     (oldIpDhcp != cfg.ip_dhcp) ||
                     (oldIp != String(cfg.ip)) ||
                     (oldMask != String(cfg.mask)) ||
                     (oldGw != String(cfg.gw)) ||
                     (oldDns != String(cfg.dns));

  // Détecter si les paramètres Balance ont changé
  bool balanceChanged =
      (oldBalanceType != cfg.balanceType) ||
      (oldBalanceBaud != cfg.balanceBaud) || (oldDataBits != cfg.dataBits) ||
      (oldParity != cfg.parity) || (oldStopBits != cfg.stopBits) ||
      (oldHandshake != cfg.handshake) || (oldSwapRxTx != cfg.swapRxTx) ||
      (oldTerminator != String(cfg.terminator));

  // Sauvegarder la configuration
  saveConfig();

  // Réinitialiser DAC2 si paramètres modifiés (toggle activé = DAC activé)
  if (cfg.dac2_enabled) {
    if (g_dac2.begin() == 0) {
      g_dac2.setDACOutRange(g_dac2.eOutputRange10V);
      dac2Ok = true;
      Serial.println("[DAC2] Activé et réinitialisé");
      // Appliquer immédiatement le poids actuel à la sortie DAC (valeur nettoyée)
      if (lastWeight.length() > 0) {
        String forDac = getWeightStringForDac(lastWeight);
        float wg = parseWeightGrams(forDac);
        uint16_t mv = weightToDacMv(wg);
        setDac2Output(mv);
        Serial.printf("[DAC2] Sortie appliquée: %s -> %u mV\n",
                      forDac.c_str(), lastDacMv);
      } else {
        setDac2Output(0);
      }
    } else {
      dac2Ok = false;
      Serial.println("[DAC2] Échec init (vérifier câblage I2C)");
    }
  } else {
    if (dac2Ok) {
      g_dac2.setDACOutVoltage(0, 0);
      g_dac2.setDACOutVoltage(0, 1);
    }
    dac2Ok = false;
    lastDacMv = 0;
  }

  if (wifiChanged) {
    // Les paramètres WiFi ont changé -> redémarrage nécessaire
    server.send(200, "text/plain",
                "Config enregistrée. Redémarrage pour appliquer les "
                "changements WiFi...");
    delay(400);
    ESP.restart();
  } else {
    // Pas de changement WiFi -> appliquer les modifications sans redémarrer
    applyUISettings();

    if (balanceChanged) {
      reinitSerial2();
      server.send(200, "text/plain",
                  "Config enregistrée. Paramètres appliqués sans redémarrage.");
    } else {
      server.send(200, "text/plain",
                  "Config enregistrée. Paramètres appliqués sans redémarrage.");
    }
  }
}

void handleFactory() {
  prefs.begin("easylog");
  prefs.clear();
  prefs.end();
  server.send(200, "text/plain", "Configuration effacée. Redémarrage...");
  delay(400);
  ESP.restart();
}

void handleStatus() {
  sendCORS();
  // Utiliser snprintf pour éviter les allocations String coûteuses
  char json[128];
  const char *balanceTypeStr = (cfg.balanceType == 0) ? "AandD" : "Sartorius";
  String nowStr = nowDateTime();
  snprintf(json, sizeof(json),
           "{\"lastWeight\":\"%s\",\"now\":\"%s\",\"balanceType\":\"%s\","
           "\"baudRate\":%lu}",
           lastDisplayedValue.c_str(), nowStr.c_str(), balanceTypeStr, cfg.balanceBaud);
  server.send(200, "application/json", json);
}

void handleTare() {
  sendCORS();
  // Envoyer la commande "T" à la balance pour la remettre à zéro
  String cmd = "T";
  cmd += String(cfg.terminator);

  // Logger dans le terminal
  addTerminalLog("TX", cmd);

  Serial2.write((const uint8_t *)cmd.c_str(), cmd.length());
  Serial.print("[API] Tare (T): ");
  Serial.println(cmd);
  server.send(200, "application/json",
              "{\"success\":true,\"message\":\"Tare command sent\"}");
}

void handleBalanceCommand() {
  sendCORS();
  if (!server.hasArg("cmd")) {
    server.send(400, "application/json",
                "{\"error\":\"Missing 'cmd' parameter\"}");
    return;
  }

  String cmd = server.arg("cmd");
  String fullCmd = cmd + String(cfg.terminator);

  // Logger dans le terminal
  addTerminalLog("TX", fullCmd);

  Serial2.write((const uint8_t *)fullCmd.c_str(), fullCmd.length());
  Serial.printf("[API] Balance command: %s", fullCmd.c_str());

  String response = "{\"success\":true,\"command\":\"";
  response += cmd;
  response += "\",\"message\":\"Command sent\"}";
  server.send(200, "application/json", response);
}

void handleMonitor() {
  sendCORS();
  // Retourner l'état actuel de l'écran et le log du terminal
  String json = "{";
  json += "\"screen\":{";
  json += "\"mode\":\"" + currentScreenMode + "\",";

  // Échapper les caractères spéciaux pour JSON
  String escapedContent = currentScreenContent;
  escapedContent.replace("\\", "\\\\");
  escapedContent.replace("\"", "\\\"");
  escapedContent.replace("\n", "\\n");
  escapedContent.replace("\r", "\\r");

  json += "\"content\":\"" + escapedContent + "\"";
  json += "},";

  // Échapper le terminal log
  String escapedTerminal = terminalLog;
  escapedTerminal.replace("\\", "\\\\");
  escapedTerminal.replace("\"", "\\\"");
  escapedTerminal.replace("\n", "\\n");
  escapedTerminal.replace("\r", "\\r");

  json += "\"terminal\":\"" + escapedTerminal + "\",";

  // Capteurs
  json += "\"temp\":" + String(getCoreTemp(), 1) + ",";
  json += "\"cpu\":" + String(cpuLoad, 0) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap() / 1024); // ko

  // Poids actuel DAC2 : toujours valeur nettoyée (jamais ST/US), fallback si lastDisplayedValue vide
  String weightForDacDisplay = lastDisplayedValue.length() > 0
                                   ? lastDisplayedValue
                                   : formatWeightForDisplay(lastWeight);
  weightForDacDisplay.replace("\\", "\\\\");
  weightForDacDisplay.replace("\"", "\\\"");
  json += ",\"weight\":\"" + weightForDacDisplay + "\"";
  json += ",\"dacMv\":" + String(lastDacMv);
  int8_t bat = getBatteryLevel();
  json += ",\"battery\":" + String((int)bat);
  json += ",\"charging\":" + String(isBatteryCharging() ? "true" : "false");
  if (currentScreenMode == "weight" && cfg.weightColorByStability) {
    json += ",\"stable\":";
    json += isWeightStable() ? "true" : "false";
  }
  json += "}";
  server.send(200, "application/json", json);
}

static void wsLoop() {
  // Accepter un nouveau client (accept() retourne un client si une connexion est en attente)
  {
    WebsocketsClient client = wsServer.accept();
    if (client.available()) {
        int slot = -1;
        for (int i = 0; i < WS_MAX_CLIENTS; i++) {
          if (!wsClientInUse[i]) {
            slot = i;
            break;
          }
        }
        if (slot >= 0) {
          wsClients[slot] = client;
          wsClientInUse[slot] = true;
          const int idx = slot;
          wsClients[slot].onEvent([idx](WebsocketsEvent e, String d) {
            if (e == WebsocketsEvent::ConnectionClosed)
              wsClientInUse[idx] = false;
          });
          wsClients[slot].onMessage([](WebsocketsMessage msg) {
            Serial.printf("[WS] 📨 Message reçu: %s\n", msg.data().c_str());
          });
          Serial.printf("[WS] ✅ Client #%d connecté\n", slot);
          if (lastDisplayedValue.length() > 0) {
            char json[128];
            String nowStr = nowDateTime();
            snprintf(json, sizeof(json),
                     "{\"weight\":\"%s\",\"timestamp\":\"%s\"}",
                     lastDisplayedValue.c_str(), nowStr.c_str());
            wsSendTXT((uint8_t)slot, json);
            Serial.printf("[WS] 📤 Envoi dernière pesée au client #%d\n",
                          slot);
          }
        }
      }
  }
  // Poll tous les clients connectés
  for (int i = 0; i < WS_MAX_CLIENTS; i++) {
    if (wsClientInUse[i]) {
      wsClients[i].poll();
      if (!wsClients[i].available()) {
        wsClientInUse[i] = false;
        Serial.printf("[WS] ⚠️  Client #%d déconnecté\n", i);
      }
    }
  }
}

// Répond à la sonde Windows NCSI (connecttest.txt) pour éviter les 404 répétés
void handleConnectTest() {
  sendCORS();
  server.send(200, "text/plain", "Microsoft Connect Test");
}

void handleNotFound() {
  Serial.printf("[HTTP] 404 Not Found: %s from %s\n", server.uri().c_str(),
                server.client().remoteIP().toString().c_str());
  sendCORS();
  server.send(404, "text/plain", "Not found");
}

void handleOptions() {
  sendCORS();
  server.send(200);
}

void handleTest() {
  sendCORS();
  IPAddress ip = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
  String response = "Server OK\n";
  response += "WiFi Mode: " + String(WiFi.getMode()) + "\n";
  response += "IP: " + ip.toString() + "\n";
  response += "Status: " + String(WiFi.status()) + "\n";
  server.send(200, "text/plain", response);
  Serial.println("[HTTP] /test called - Server is responding");
}

void setupWeb() {
  // Endpoint de test simple pour vérifier que le serveur répond
  server.on("/test", HTTP_GET, handleTest);
  // Sonde Windows NCSI - évite les 404 répétés dans les logs
  server.on("/connecttest.txt", HTTP_GET, handleConnectTest);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/factory", HTTP_POST, handleFactory);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/tare", HTTP_POST, handleTare);
  server.on("/api/balance/command", HTTP_POST, handleBalanceCommand);
  server.on("/api/monitor", HTTP_GET, handleMonitor);
  server.onNotFound(handleNotFound);

  // Gérer les requêtes OPTIONS pour CORS
  server.on("/test", HTTP_OPTIONS, handleOptions);
  server.on("/", HTTP_OPTIONS, handleOptions);
  server.on("/save", HTTP_OPTIONS, handleOptions);
  server.on("/factory", HTTP_OPTIONS, handleOptions);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/tare", HTTP_OPTIONS, handleOptions);
  server.on("/api/balance/command", HTTP_OPTIONS, handleOptions);
  server.on("/api/monitor", HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("[HTTP] Server started on port 80");
  Serial.println("[HTTP] Test endpoint: http://<IP>/test");

  // Démarrer le serveur WebSocket (WebSockets2_Generic)
  wsServer.listen(81);
  Serial.println("[WS] WebSocket server started on port 81");
}

// ===== Dessin écran (simplifié) =====
void drawWifiIndicator() {
  if (!cfg.showWifiIndicator)
    return;
  auto &d = AtomS3.Display;
  uint16_t color = RED;
  String label = "WiFi";

  if (wifiOK && timeReady)
    color = GREEN;
  else if (wifiOK)
    color = YELLOW;

  int yPos = d.height() - 8;
  d.fillRoundRect(2, yPos, 8, 8, 2, color);
  d.setTextColor(WHITE);
  d.setTextDatum(top_left);
  d.setTextSize(1);
  d.drawString(label, 12, yPos);
}

void drawHeader() {
  auto &d = AtomS3.Display;
  d.setTextDatum(top_center);
  d.setTextColor(WHITE);
  d.setTextSize(1);
  d.drawString(cfg.title, d.width() / 2, 4);
  d.drawLine(4, 18, d.width() - 4, 18, WHITE);
}

void drawMainCentered(const String &text, uint16_t color) {
  auto &d = AtomS3.Display;
  d.setTextDatum(middle_center);
  d.setTextColor(color);
  d.setTextSize(cfg.fontMain);
  d.drawString(text, d.width() / 2, d.height() / 2 - 2);
}

void drawIP() {
  auto &d = AtomS3.Display;
  IPAddress ip;
  if (WiFi.getMode() == WIFI_AP) {
    ip = WiFi.softAPIP();
  } else {
    ip = WiFi.localIP();
  }

  if (ip == IPAddress(0, 0, 0, 0)) {
    return; // Pas d'IP à afficher
  }

  String ipStr = ip.toString();
  d.setTextColor(YELLOW);
  d.setTextDatum(top_center);
  d.setTextSize(1);
  d.drawString(ipStr, d.width() / 2, 22);
}

void drawDateTime() {
  auto &d = AtomS3.Display;
  int yStart = 90;
  int yHeight = d.height() - yStart - 10;
  d.fillRect(0, yStart, d.width(), yHeight, BLACK);
  d.setTextColor(CYAN);
  d.setTextDatum(middle_center);
  d.setTextSize(1);
  d.drawString(nowDateTime(), d.width() / 2, yStart + yHeight / 2);
}

void drawIdleScreen() {
  auto &d = AtomS3.Display;
  d.clear();
  drawHeader();
  drawIP();
  drawMainCentered(cfg.idleText, COL_GRAY);
  drawDateTime();
  drawWifiIndicator();

  // Mettre à jour l'état pour l'interface web (seulement si des clients sont
  // connectés)
  static uint32_t lastScreenUpdate = 0;
  if (wsConnectedCount() > 0 && (millis() - lastScreenUpdate > 500)) {
    updateScreenState();
    lastScreenUpdate = millis();
  }
}

// Formate la valeur poids pour l'affichage : retire toujours ST,/US, (non affichés),
// masque zéros non significatifs, option signe +
String formatWeightForDisplay(const String &raw) {
  String out = raw;
  if (out.startsWith("ST,") || out.startsWith("ST "))
    out = out.substring(3);
  else if (out.startsWith("US,") || out.startsWith("US "))
    out = out.substring(3);
  if (cfg.weightHideLeadingZeros) {
    int i = 0;
    if (out.length() > 0 && (out.charAt(0) == '+' || out.charAt(0) == '-'))
      i = 1;
    while (i < out.length() && out.charAt(i) == '0' && i + 1 < out.length() &&
           out.charAt(i + 1) != '.' && out.charAt(i + 1) != ',')
      i++;
    if (i > 0 && (out.charAt(0) == '+' || out.charAt(0) == '-'))
      out = out.substring(0, 1) + out.substring(i);
    else if (i > 0)
      out = out.substring(i);
  }
  if (!cfg.weightShowPlusSign && out.length() > 0 && out.charAt(0) == '+')
    out = out.substring(1);
  return out;
}

void drawWeightScreen(const String &displayOrRaw) {
  auto &d = AtomS3.Display;
  d.clear();
  drawHeader();
  drawIP();
  // Reformat au cas où on reçoit du brut ; si déjà formaté (pas de ST,/US,) inchangé
  String displayStr = formatWeightForDisplay(displayOrRaw);
  uint16_t color = WHITE;
  if (cfg.weightColorByStability) {
    color = isWeightStable() ? COL_STABLE : COL_UNSTABLE;
  }
  drawMainCentered(displayStr, color);
  // Sous le poids : tension DAC (mV) si DAC2 activé
  if (cfg.dac2_enabled) {
    char dacStr[24];
    snprintf(dacStr, sizeof(dacStr), "%u mV", lastDacMv);
    d.setTextDatum(middle_center);
    d.setTextColor(CYAN);
    d.setTextSize(1);
    d.drawString(dacStr, d.width() / 2, d.height() / 2 + 20);
  }
  drawDateTime();
  drawWifiIndicator();

  currentScreenMode = "weight";
  currentScreenContent = displayStr;
}

// Retourne la valeur nettoyée pour le DAC (sans préfixe ST,/US,) — à utiliser
// uniquement pour parseWeightGrams / DAC, pas pour l'affichage.
String getWeightStringForDac(const String &cleaned) {
  String out = cleaned;
  if (out.startsWith("ST,") || out.startsWith("ST "))
    out = out.substring(3);
  else if (out.startsWith("US,") || out.startsWith("US "))
    out = out.substring(3);
  return out;
}

// Extraire la valeur numérique (g) depuis "123.45 g", "N 0.0 g", "+ 12.34 g"
float parseWeightGrams(const String &s) {
  int start = -1, end = -1;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    bool isNum =
        (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == ',';
    if (isNum && start < 0)
      start = (int)i;
    if (start >= 0 && !isNum) {
      end = (int)i;
      break;
    }
    if (start >= 0 && i == s.length() - 1)
      end = (int)i + 1;
  }
  if (start < 0)
    return 0.0f;
  if (end < 0)
    end = (int)s.length();
  String t = s.substring(start, end);
  t.replace(",", ".");
  return t.toFloat();
}

// Convertir poids (g) en mV via calibration 6 points (interpolation linéaire
// par segments)
uint16_t weightToDacMv(float weightG) {
  if (weightG < cfg.dac2_range_min)
    return cfg.dac2_cal_mv[0];
  if (weightG > cfg.dac2_range_max)
    return cfg.dac2_cal_mv[5];
  if (weightG <= cfg.dac2_cal_weight[0])
    return cfg.dac2_cal_mv[0];
  if (weightG >= cfg.dac2_cal_weight[5])
    return cfg.dac2_cal_mv[5];
  for (int i = 0; i < 5; i++) {
    if (weightG >= cfg.dac2_cal_weight[i] &&
        weightG <= cfg.dac2_cal_weight[i + 1]) {
      float w0 = cfg.dac2_cal_weight[i], w1 = cfg.dac2_cal_weight[i + 1];
      float m0 = (float)cfg.dac2_cal_mv[i], m1 = (float)cfg.dac2_cal_mv[i + 1];
      if (w1 <= w0)
        return cfg.dac2_cal_mv[i];
      float t = (weightG - w0) / (w1 - w0);
      return (uint16_t)(m0 + t * (m1 - m0));
    }
  }
  return cfg.dac2_cal_mv[0];
}

void setDac2Output(uint16_t mv) {
  if (!dac2Ok || !cfg.dac2_enabled)
    return;
  if (mv > 10000)
    mv = 10000;
  lastDacMv = mv;
  uint16_t dacVal = (uint32_t)mv * 32767UL / 10000UL;
  if (dacVal > 32767)
    dacVal = 32767;
  g_dac2.setDACOutVoltage(dacVal, 0);
  g_dac2.setDACOutVoltage(dacVal, 1);
}

String parseBalanceResponse(const String &raw) {
  String cleaned = raw;
  cleaned.replace("\r", "");
  cleaned.replace("\n", "");
  cleaned.trim();

  if (cfg.balanceType == (uint8_t)BalanceType::Sartorius &&
      cleaned.length() > 0) {
    String result = "";
    for (size_t i = 0; i < cleaned.length(); i++) {
      char c = cleaned.charAt(i);
      if (isprint(c) || c == ' ') {
        result += c;
      }
    }
    result.trim();
    return result.length() > 0 ? result : cleaned;
  }

  return cleaned;
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  auto m5cfg = M5.config();
  AtomS3.begin(m5cfg);
  AtomS3.Display.setRotation(0);

  loadConfig();

  Serial.printf("\n\n=== Easylogger v2.0 ===\n");
  Serial.println("ℹ️  En cas d'échec WiFi, l'AtomS3 démarrera en mode AP "
                 "(réseau Easylogger-XXXX)");

  // Essayer WiFi client, sinon AP
  bool sta = startClientWiFi();
  if (!sta) {
    Serial.println("[WiFi] Démarrage en mode AP (autonome)");
    startAP(); // Toujours démarrer l'AP si pas de WiFi client - fonctionnement
               // autonome garanti
  }

  // Vérifier que le WiFi est bien configuré avant de démarrer le serveur
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP) {
    Serial.println("[HTTP] WiFi OK, démarrage du serveur web...");
    setupWeb();

    // Afficher l'IP pour faciliter l'accès
    IPAddress ip =
        (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
    Serial.printf("[HTTP] ✅ Serveur web accessible sur: http://%s\n",
                  ip.toString().c_str());
    if (WiFi.getMode() == WIFI_AP) {
      Serial.printf(
          "[HTTP] 📶 Connectez-vous au réseau: %s (mot de passe: easylogger)\n",
          currentAPSSID.c_str());
    }
  } else {
    Serial.println("[HTTP] ⚠️  WiFi non configuré - serveur web non démarré");
  }

  uint32_t baud = getBalanceBaud();
  int serialConfig = getBalanceSerialConfig();
  int rxPin = getBalanceRxPin();
  int txPin = getBalanceTxPin();
  Serial2.begin(baud, serialConfig, rxPin, txPin);

  char parityChar = (cfg.parity == 1) ? 'E' : ((cfg.parity == 2) ? 'O' : 'N');
  char configStr[8];
  snprintf(configStr, sizeof(configStr), "%d%c%d", cfg.dataBits, parityChar,
           cfg.stopBits);

  Serial.printf(
      "[Balance] Type: %s, Baud: %lu, Config: %s, RX: %d, TX: %d\n",
      (cfg.balanceType == (uint8_t)BalanceType::AandD ? "A&D" : "Sartorius"),
      baud, configStr, rxPin, txPin);

  // Initialiser la balance Sartorius en mode continu si nécessaire
  if (cfg.balanceType == (uint8_t)BalanceType::Sartorius) {
    delay(1000); // Attendre que Serial2 soit complètement initialisé
    initSartoriusContinuous();
  }

  // Initialiser DAC2 (Unit DAC2 GP8413) si activé
  if (cfg.dac2_enabled) {
    if (g_dac2.begin() == 0) {
      g_dac2.setDACOutRange(g_dac2.eOutputRange10V);
      g_dac2.setDACOutVoltage(0, 0);
      g_dac2.setDACOutVoltage(0, 1);
      dac2Ok = true;
      Serial.println("[DAC2] GP8413 OK - Plage 0-10V, 15 bits");
    } else {
      Serial.println("[DAC2] Init échec - Vérifier le câblage I2C (Unit DAC2)");
    }
  }

  drawIdleScreen();

  // Initialiser l'état de l'écran pour l'interface web
  updateScreenState();

  Serial.println("[READY]");
}

// ===== Loop =====
void loop() {
  updateCpuLoad(); // Mesure de la charge
  AtomS3.update();

  // Serveur HTTP - Appeler plusieurs fois pour une meilleure réactivité
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP) {
    server.handleClient();
    server.handleClient(); // Double appel pour traiter toutes les requêtes
  }

  // Traiter WebSocket (accept + poll clients)
  wsLoop();
  wsLoop();
  wsLoop();
  wsLoop();

  // Envoi périodique stats système (1s) + poids et DAC pour page paramètres DAC
  static uint32_t lastSysStats = 0;
  if (millis() - lastSysStats > 1000 && wsConnectedCount() > 0) {
    String weightForDacDisplay = lastDisplayedValue.length() > 0
                                     ? lastDisplayedValue
                                     : formatWeightForDisplay(lastWeight);
    weightForDacDisplay.replace("\\", "\\\\");
    weightForDacDisplay.replace("\"", "\\\"");
    int8_t bat = getBatteryLevel();
    String json = "{\"type\":\"sys\",\"temp\":" + String(getCoreTemp(), 1) +
                  ",\"cpu\":" + String(cpuLoad, 0) +
                  ",\"heap\":" + String(ESP.getFreeHeap() / 1024) +
                  ",\"weight\":\"" + weightForDacDisplay + "\"" +
                  ",\"dacMv\":" + String(lastDacMv) +
                  ",\"battery\":" + String((int)bat) +
                  ",\"charging\":" + String(isBatteryCharging() ? "true" : "false");
    if (cfg.weightColorByStability) {
      json += ",\"stable\":";
      json += isWeightStable() ? "true" : "false";
    }
    json += "}";
    wsBroadcastTXT(json.c_str());
    lastSysStats = millis();
  }

  if (dnsRunning)
    dnsServer.processNextRequest();

  // Bouton - Détection appui long (8 secondes) pour reset usine
  if (AtomS3.BtnA.isPressed()) {
    if (!btnPressed) {
      // Début de l'appui
      btnPressed = true;
      btnPressStart = millis();
      Serial.println("[BTN] Appui détecté...");
    } else {
      // Appui en cours - vérifier si on atteint 8 secondes
      uint32_t pressDuration = millis() - btnPressStart;
      if (pressDuration >= LONG_PRESS_MS) {
        Serial.println("[BTN] Appui long détecté (8s) - Reset usine");
        btnPressed = false;
        // Réinitialiser aux paramètres d'usine
        prefs.begin("easylog");
        prefs.clear();
        prefs.end();
        // Afficher message sur l'écran
        auto &d = AtomS3.Display;
        d.clear();
        d.setTextDatum(middle_center);
        d.setTextColor(RED);
        d.setTextSize(1);
        d.drawString("RESET USINE", d.width() / 2, d.height() / 2 - 10);
        d.drawString("Redemarrage...", d.width() / 2, d.height() / 2 + 10);
        delay(1000);
        ESP.restart();
      }
    }
  } else {
    // Bouton relâché
    if (btnPressed) {
      uint32_t pressDuration = millis() - btnPressStart;
      if (pressDuration < LONG_PRESS_MS) {
        // Appui court - envoie "T" pour remettre la balance à zéro
        String cmd = "T";
        cmd += String(cfg.terminator);
        addTerminalLog("TX", cmd);
        Serial2.write((const uint8_t *)cmd.c_str(), cmd.length());
        Serial.print("[TX] Tare (T): ");
        Serial.println(cmd);
      }
      btnPressed = false;
    }
  }

  // Réception balance
  bool gotLine = false;
  while (Serial2.available() > 0) {
    char c = (char)Serial2.read();
    if (c == '\n') {
      if (lineLen > 0) {
        lineBuf[lineLen] = '\0';
        String s = String(lineBuf);
        s.replace("\r", "");
        s.replace("\n", "");
        s.trim();

        if (s.length() > 0) {
          String cleaned = parseBalanceResponse(s);

          lastWeight = cleaned;
          lastWeightMs = millis();
          String displayStr = formatWeightForDisplay(cleaned);
          lastDisplayedValue = displayStr;  // Une seule source pour l'affichage (jamais ST/US)
          if (displayStr != lastFormattedValue) {
            lastFormattedValue = displayStr;
            lastFormattedValueSince = millis();
          }

          // Poids -> mV : utiliser la valeur nettoyée pour le DAC (sans ST/US)
          if (cfg.dac2_enabled) {
            String forDac = getWeightStringForDac(cleaned);
            float wg = parseWeightGrams(forDac);
            uint16_t mv = weightToDacMv(wg);
            lastDacMv = mv;  // Affichage web même si matériel non connecté
            if (dac2Ok)
              setDac2Output(mv);  // Envoyer au DAC seulement si init OK
          }

          // Envoyer chaque pesée via WebSocket (poids brut pour données)
          uint8_t clients = wsConnectedCount();
          if (clients > 0) {
            bool st = cfg.weightColorByStability ? isWeightStable() : false;
            char json[160];
            if (cfg.weightColorByStability) {
              snprintf(json, sizeof(json), "{\"weight\":\"%s\",\"dacMv\":%u,\"stable\":%s}",
                       lastDisplayedValue.c_str(), lastDacMv, st ? "true" : "false");
            } else {
              snprintf(json, sizeof(json), "{\"weight\":\"%s\",\"dacMv\":%u}",
                       lastDisplayedValue.c_str(), lastDacMv);
            }
            wsBroadcastTXT(json);

            // Écran web : valeur formatée + stable pour couleur, throttle 20 Hz
            if (millis() - lastScreenSendMs >= SCREEN_SEND_INTERVAL_MS) {
              lastScreenSendMs = millis();
              char screenJson[160];
              if (cfg.weightColorByStability) {
                snprintf(screenJson, sizeof(screenJson),
                         "{\"type\":\"screen\",\"mode\":\"weight\",\"content\":\"%s\",\"stable\":%s}",
                         lastDisplayedValue.c_str(), st ? "true" : "false");
              } else {
                snprintf(screenJson, sizeof(screenJson),
                         "{\"type\":\"screen\",\"mode\":\"weight\",\"content\":\"%s\"}",
                         lastDisplayedValue.c_str());
              }
              wsBroadcastTXT(screenJson);
            }

            wsLoop();
            wsLoop();
            wsLoop();
          }

          // Écran physique : valeur formatée, 20 FPS max (20 pesées/s)
          static uint32_t lastScreenDraw = 0;
          if (millis() - lastScreenDraw >= SCREEN_SEND_INTERVAL_MS) {
            drawWeightScreen(lastDisplayedValue);
            lastScreenDraw = millis();
          }

          // Logger terminal seulement si demandé
          if (wsConnectedCount() > 0) {
            addTerminalLog("RX", String(lineBuf));
          }

          uiMode = UiMode::ShowWeight;
          modeEnteredAt = millis();
          gotLine = true;
        }
        lineLen = 0;
      }
    } else if (c == '\r' || (c >= 32 && c <= 126)) {
      if (lineLen < sizeof(lineBuf) - 1) {
        lineBuf[lineLen++] = c;
      } else {
        lineLen = 0;
      }
    } else {
      if (cfg.balanceType == (uint8_t)BalanceType::Sartorius &&
          lineLen < sizeof(lineBuf) - 1) {
        lineBuf[lineLen++] = c;
      }
    }
    lastByteMs = millis();
  }

  // Timeout
  if (lineLen > 0 && (millis() - lastByteMs >= LINE_TIMEOUT_MS)) {
    lineBuf[lineLen] = '\0';
    String s = String(lineBuf);
    s.replace("\r", "");
    s.replace("\n", "");
    s.trim();
    if (s.length() > 0) {
      String cleaned = parseBalanceResponse(s);

      String displayStrTimeout = formatWeightForDisplay(cleaned);
      lastDisplayedValue = displayStrTimeout;
      if (displayStrTimeout != lastFormattedValue) {
        lastFormattedValue = displayStrTimeout;
        lastFormattedValueSince = millis();
      }

      // Logger dans le terminal (RX - timeout) - seulement si des clients
      // WebSocket sont connectés
      uint8_t wsClientsTimeout = wsConnectedCount();
      if (wsClientsTimeout > 0) {
        String rawData = String(lineBuf);
        addTerminalLog("RX", rawData);
      }

      lastWeight = cleaned;
      lastWeightMs = millis();

      // Poids -> mV : utiliser la valeur nettoyée pour le DAC (sans ST/US)
      if (cfg.dac2_enabled) {
        String forDac = getWeightStringForDac(cleaned);
        float wg = parseWeightGrams(forDac);
        uint16_t mv = weightToDacMv(wg);
        lastDacMv = mv;  // Affichage web même si matériel non connecté
        if (dac2Ok)
          setDac2Output(mv);  // Envoyer au DAC seulement si init OK
      }

      // WebSocket : poids brut + écran formaté (throttle 20 Hz)
      uint8_t clients = wsConnectedCount();
      if (clients > 0) {
        bool st = cfg.weightColorByStability ? isWeightStable() : false;
        char json[160];
        if (cfg.weightColorByStability) {
          snprintf(json, sizeof(json), "{\"weight\":\"%s\",\"dacMv\":%u,\"stable\":%s}",
                   lastDisplayedValue.c_str(), lastDacMv, st ? "true" : "false");
        } else {
          snprintf(json, sizeof(json), "{\"weight\":\"%s\",\"dacMv\":%u}",
                   lastDisplayedValue.c_str(), lastDacMv);
        }
        wsBroadcastTXT(json);

        if (millis() - lastScreenSendMs >= SCREEN_SEND_INTERVAL_MS) {
          lastScreenSendMs = millis();
          char screenJson[160];
          if (cfg.weightColorByStability) {
            snprintf(screenJson, sizeof(screenJson),
                     "{\"type\":\"screen\",\"mode\":\"weight\",\"content\":\"%s\",\"stable\":%s}",
                     lastDisplayedValue.c_str(), st ? "true" : "false");
          } else {
            snprintf(screenJson, sizeof(screenJson),
                     "{\"type\":\"screen\",\"mode\":\"weight\",\"content\":\"%s\"}",
                     lastDisplayedValue.c_str());
          }
          wsBroadcastTXT(screenJson);
        }

        wsLoop();
        wsLoop();
        wsLoop();
      }

      // Écran physique : valeur formatée, 20 Hz max
      static uint32_t lastScreenDrawTimeout = 0;
      if (millis() - lastScreenDrawTimeout >= SCREEN_SEND_INTERVAL_MS) {
        drawWeightScreen(lastDisplayedValue);
        lastScreenDrawTimeout = millis();
      }

      uiMode = UiMode::ShowWeight;
      modeEnteredAt = millis();
    }
    lineLen = 0;
  }

  // Rafraîchissement UI
  if (uiMode == UiMode::ShowWeight) {
    if (millis() - modeEnteredAt >= cfg.showWeightMs) {
      uiMode = UiMode::IdleClock;
      modeEnteredAt = millis();
      drawIdleScreen();
    } else {
      // Redessin périodique (100 ms) pour réévaluer la couleur stabilité (vert/orange)
      static uint32_t lastWeightRedraw = 0;
      if (millis() - lastWeightRedraw >= 100) {
        lastWeightRedraw = millis();
        drawWeightScreen(lastDisplayedValue);
      }
    }
  } else if (uiMode == UiMode::IdleClock) {
    static uint32_t lastIdle = 0;
    if (millis() - lastIdle >= IDLE_REFRESH_MS) {
      lastIdle = millis();
      drawIdleScreen();
    }
  }
}
