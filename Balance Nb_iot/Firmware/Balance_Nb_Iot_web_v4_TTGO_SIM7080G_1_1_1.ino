/*
  Balance_Nb_Iot_web_v4_TTGO_SIM7080G_mqtt_gps_V1.1.1.ino

  CORRECTIONS (2026-04-20) — v1.1.1 :
  ✅ Commande MQTT set_config : change serveur et/ou port MQTT à distance
     → Payload : {"action":"set_config","port":1886,"server":"nouveau.domaine.fr"}
     → Les deux champs sont optionnels (l'un ou l'autre suffit)
     → Sauvegarde en NVS, efface le retained /cmd, ACK, puis redémarrage
     → Disponible via le bouton ⚙️ Config MQTT du FOTA UI
  ✅ Ajout ICCID (numéro de carte SIM) dans le message JSON diagnostics
     → Lecture manuelle AT+CCID (getSimCCID() retournait "OK" sur SIM7080G)
     → Stocké dans la variable globale simICCID (String)
     → Publié dans le topic diagnostics : {"firmware":...,"iccid":"89330..."}
     → Permet d'identifier la carte SIM associée à chaque balance à distance

  CORRECTIONS (2026-04-13) — v1.1.0 :
  ✅ Pages "Calibration Réussie" (balance 1 et balance 2) :
     → Boutons "Tester la balance" et "Retour au menu" enveloppés dans un div flex
     → display:flex + gap:12px + flex-wrap:wrap + justify-content:center
     → Affichage correct sur smartphone (plus de débordement/chevauchement)

  CORRECTIONS (2026-03-10) :
  ✅ PMU DC3 réactivé : PMU.setDC3Voltage(3000) + PMU.enableDC3() dans
     setupNormalInit(), setupAPModeInit() et stabilizePower()
     → Alimentation SIM7080G restaurée (modem ne répondait plus à AT)
  ✅ Ack FOTA/Reset non-retained : retained=false sur topic /cmd/ack
     → Le message d'accusé de réception ne persiste plus sur le broker
  ✅ Témoin visuel FOTA (fota.cpp) : clignotement LED pendant le download
     → 1 flash court (150ms) + 1 flash long (400ms) par seconde
     → LED éteinte à la fin (succès ou échec)

  AJOUT RESET + FOTA (2026-03-07) — backport depuis v1.0.16 :
  ✅ Reset à distance via MQTT : {"action":"reset"} sur topic /cmd
     → Remet à zéro crash_count + fota_retry en NVS (pas de redémarrage)
  ✅ Nouveaux fichiers : fota.h, fota.cpp (copiés depuis v1.0.16)
     → Téléchargement firmware via TCP mux0 (TinyGsmClient)
     → Plus de conflit de mux : mqtt.disconnect() + client.stop() avant fotaExecute
     → Vérification MD5 + taille avant reboot
     → Compteur de tentatives NVS (max 10)
  ✅ mqttCallback() : parse la commande JSON {"action":"ota","url":"...","md5":"...","size":...}
  ✅ safeMQTTConnect() : setBufferSize(512) + setCallback(mqttCallback)
  ✅ Topic commande : deviceId + "/cmd"  (ex: E0D16CE6FC84/cmd)
  ✅ Topic ack : deviceId + "/cmd/ack"
  ✅ Bloc FOTA dans mainLoop() : subscribe 3s → execute → ack → restart si succès
  ✅ Commande reset : {"action":"reset"} → remet à zéro crash_count + fota_retry en NVS
     → Pas de redémarrage, le cycle continue normalement après le reset
  ✅ Stack appTask : 10000 → 20000 octets
  ✅ Page web /fota : monitoring état FOTA + reset compteur NVS + lien menu principal

  CORRECTION (2026-03-05):
  ✅ Calibration en 2 étapes avec tare automatique :
     → Étape 1 : balance vide → tare automatique (raw_zero mesuré et transmis)
     → Étape 2 : poser le poids connu → calcul du facteur avec le raw_zero de l'étape 1
     → Le raw_zero est également stocké comme offset (plus besoin de faire la tare séparément)
     → Suppression de la dépendance à l'offset précédemment stocké lors de la calibration
  ✅ Migration vers méthodes natives bibliothèque HX711 (Rob Tillaart v0.6.3) :
     → Calibration : tare(20) + calibrate_scale(poids, 20) → plus précis (20 lectures vs 10)
     → Calibration réduite de 3 pages à 2 pages (tare automatique sur chargement)
     → Tare manuelle : tare(10) au lieu de read_average(10) manuel
     → Mesure : get_units(10) au lieu de (raw - offset) / factor manuel
     → Initialisation : set_offset() ajouté après set_scale() au démarrage

  CHANGEMENTS v1.0.13:
  ✅ Configuration réseau NB-IoT/LTE-M explicite dans safeModemStart()
     → AT+CFUN=0 (mode avion) avant configuration
     → AT+CMNB=2 (NB-IoT uniquement, forfait Bouygues NB-IoT)
     → AT+CBANDCFG="NB-IOT",20 (bande B20 800MHz Bouygues)
     → AT+CGDCONT=1,"IP","" (APN vide, attribué auto par réseau: ido.net)
     → AT+CEREG=2 (infos étendues: TAC, Cell ID, AcT)
     → AT+CFUN=1 puis AT+COPS=0 (réactivation radio + opérateur auto)
  ✅ safeNetworkConnect() : surveillance CEREG directe (NB-IoT/LTE-M)
     → Parse manuelle de +CEREG pour détecter stat=1 (home) ou stat=5 (roaming)
     → Fallback TinyGSM getRegistrationStatus() en parallèle
     → Polling toutes les 5s avec messages de debug
  ✅ AT+CNETLIGHT=0 : désactivation LED modem
  ✅ AT+CMEE=2 : erreurs AT verbose pour diagnostic
  ✅ Gestion PDP auto-activé : détecte si +APP PDP: 0,ACTIVE après l'attach
     → Vérifie isGprsConnected() avant de tenter CNACT=0,1
     → Vide le buffer URC avant les commandes CNACT
     → Reset CNACT=0,0 avant retry si état incohérent

  CHANGEMENTS v1.0.12:
  ✅ Authentification MQTT : credentials générés automatiquement depuis eFuse MAC
     → Username : "bal_" + 8 caractères hex du MAC (ex: "bal_A1B2C3D4")
     → Password : hash déterministe 16 caractères hex basé sur le MAC complet
     → Reproductible : mêmes credentials à chaque boot pour un même ESP32
     → Non modifiable : sécurité par design, pas de mot de passe en dur
  ✅ mqtt_user / mqtt_pass : changés de const char* vides à String générés
  ✅ safeMQTTConnect() : utilise les credentials générés (au lieu de chaînes vides)
  ✅ Interface web : affiche les identifiants MQTT (user + password) sur la page d'accueil
  ✅ Page config MQTT : affiche les credentials en lecture seule avec instructions
     → Permet de copier facilement user/pass pour configurer le broker MQTT

  CHANGEMENTS v1.0.11:
  ✅ ID unique station : basé sur eFuse MAC ESP32-S3 (12 caractères hex)
  ✅ Topics MQTT : format deviceId/deviceName/<>/topic/<>/data/0
     → deviceId garantit l'unicité (filtrage Telegraf)
     → deviceName reste modifiable pour la lisibilité
  ✅ WiFi SSID : "balance" + deviceName (lisible, modifiable)
  ✅ Interface web : affiche ID station (fixe) + nom station (modifiable)
  ✅ Évite collisions : plusieurs stations peuvent avoir le même nom

  CHANGEMENTS v1.0.10 (Version finale pour déploiement):
  ✅ Watchdog optimisé : suppression tâches surveillance inutiles
  ✅ TWDT timeout 6 min (pour GPS 3 min max)
  ✅ Deep sleep 12h conservé (économie batterie optimale)
  ✅ Heartbeat simplifié : 2× par cycle (réveil + avant sleep)
  ✅ Preferences : gestion mode RO/RW corrigée
  ✅ Preferences : ReadOnly par défaut pour lectures
  ✅ Preferences : Écritures boot_count réduites (8× → 1×/jour)
  ✅ Preferences : Récupération automatique si corruption NVS
  ✅ Preferences : Validation après écritures critiques
  ✅ Validation données capteurs (NaN, Inf, ranges)
  ✅ Diagnostics MQTT améliorés
  ✅ DUMP_AT_COMMANDS conservé (debug essentiel)

*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include "utilities.h"

XPowersPMU PMU;

// See all AT commands, if wanted
// ⚠️ DÉSACTIVÉ pour déploiement terrain (pas d'accès Serial)
// Économie : ~2-3 KB RAM + ~5% CPU
//#define DUMP_AT_COMMANDS

#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_MODEM_SIM7080

#include <TinyGsmClient.h>
#include <PubSubClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(Serial1, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(Serial1);
#endif

TinyGsmClient client(modem);
PubSubClient mqtt(client);

#include "WiFi.h"
#include "WebServer.h"
#include "Preferences.h"
#include "DS18B20.h"
#include "HX711.h"
#include "OneWire.h"
#include "Update.h"
#include "fota.h"

// ---------- CONSTANTS ----------
const char *ssidPrefix = "balance";
const char *password1 = "123456789";  // ⚠️ CHANGEZ AVANT DÉPLOIEMENT
const char *FIRMWARE_VERSION = "1.1.1";
WebServer server(80);
Preferences preferences;

#define ONE_WIRE_BUS 21
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

HX711 scale1, scale2;
const uint8_t dataPin1 = 12;
const uint8_t clockPin1 = 11;
const uint8_t dataPin2 = 14;
const uint8_t clockPin2 = 13;

extern const int ledPin = 1;
const int wifiPin = 47;
const int heartbeatPin = 10;

// Watchdog & tasks
TaskHandle_t appTaskHandle = NULL;

// MQTT / prefs
String mqtt_user = "";  // Généré à partir du MAC eFuse
String mqtt_pass = "";  // Généré à partir du MAC eFuse
String simICCID  = "";  // Numéro de carte SIM (lu après init modem)
String mqtt_client_id = "";
String deviceId = "";  // ID unique basé sur l'eFuse MAC de l'ESP32
String mqtt_topic_temp1, mqtt_topic_scaleA, mqtt_topic_scaleB;
String mqtt_topic_battery, mqtt_topic_signal, mqtt_topic_position, mqtt_topic_position_approx;
String mqtt_topic_diagnostics;
String mqtt_topic_cmd;   // Commandes FOTA : deviceId/cmd

// FOTA / CMD — état en attente (rempli par mqttCallback)
bool     fotaPending       = false;
bool     resetPending      = false;
bool     configPending     = false;   // set_config : port et/ou serveur
int      pendingMqttPort   = 0;       // 0 = inchangé
String   pendingMqttServer = "";      // "" = inchangé
String   fotaUrl          = "";
String   fotaMd5          = "";
uint32_t fotaExpectedSize = 0;

// globals & telemetry
int batteryPercent = 0;
int signalStrength = 0;
bool gpsDataAvailable = false;
long lat_e6 = 0;
long lon_e6 = 0;

// Compteurs pour diagnostics
uint32_t crashCount = 0;
String lastCrashType = "";

// ---------- Global variables ----------
SemaphoreHandle_t preferencesSemaphore = NULL;
SemaphoreHandle_t setupCompleteSemaphore = NULL;
bool preferencesOpen = false;
bool preferencesReadOnly = true;  // ✅ AJOUT : Mémoriser le mode d'ouverture
bool setupComplete = false;

// ✅ AJOUT : Récupération NVS
bool nvs_corrupted = false;
uint8_t nvs_recovery_attempts = 0;

// ---------- Config structure ----------
struct Config {
  float factor1, factor2;
  float offset1, offset2;
  String deviceName;
  String mqtt_server;
  int mqtt_port;
};

Config cfg;

// Timeouts & retries
const int MODEM_MAX_RETRIES = 3;
const unsigned long NETWORK_TIMEOUT = 120000UL;
const unsigned long MQTT_CONNECT_TIMEOUT = 60000UL;
const unsigned long GPS_TIMEOUT = 180000UL;
const unsigned long PUBLISH_TIMEOUT = 30000UL;
const unsigned long SCALE_READY_TIMEOUT = 500UL;
const uint64_t uS_TO_S_FACTOR = 1000000ULL;

// TWDT timeout : 6 min (GPS 3 min + réseau 2 min + marge)
const uint32_t TWDT_TIMEOUT_MS = 360000;

// Sécurité
const size_t MIN_FREE_HEAP = 32 * 1024;

// Limites de validation pour capteurs
const int MIN_VALID_WEIGHT = -100;
const int MAX_VALID_WEIGHT = 200000;
const float MAX_VALID_FACTOR = 1000000.0f;

// ---------- Utility ----------

// Génère l'ID unique du device basé sur l'eFuse MAC de l'ESP32-S3
// Format: 12 caractères hexadécimaux (ex: "A1B2C3D4E5F6")
String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  snprintf(id, sizeof(id), "%04X%08X",
           (uint16_t)(chipid >> 32),
           (uint32_t)chipid);
  return String(id);
}

// Génère les credentials MQTT uniques basés sur l'eFuse MAC de l'ESP32
// Ces credentials sont déterministes et reproductibles pour un même ESP32
// Format username: bal_XXXXXXXX (12 chars max pour compatibilité MQTT)
// Format password: hash hexadécimal de 16 caractères
void generateMQTTCredentials(String &username, String &password) {
  uint64_t chipid = ESP.getEfuseMac();

  // Utiliser les 12 caractères de l'eFuse MAC (au lieu de 8)
  char user[17];
  snprintf(user, sizeof(user), "bal_%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  username = String(user);

  // Génération du mot de passe (inchangée)
  uint32_t hash1 = (uint32_t)(chipid ^ (chipid >> 32));
  uint32_t hash2 = ((hash1 << 13) | (hash1 >> 19)) ^ 0xDEADBEEF;
  uint32_t hash3 = ((hash2 << 7) | (hash2 >> 25)) ^ (uint32_t)(chipid >> 16);
  uint32_t hash4 = hash1 ^ hash2 ^ hash3 ^ 0xCAFEBABE;

  char pass[17];
  snprintf(pass, sizeof(pass), "%08X%08X", hash3, hash4);
  password = String(pass);
}


float randomFloat(float minVal, float maxVal) {
  return minVal + (float)esp_random() / (float)UINT32_MAX * (maxVal - minVal);
}

// Validation des données
bool isValidFloat(float val) {
  return !isnan(val) && !isinf(val);
}

bool isValidFactor(float factor) {
  return isValidFloat(factor) && factor != 0.0f && abs(factor) < MAX_VALID_FACTOR;
}

bool isValidWeight(int weight) {
  return (weight >= MIN_VALID_WEIGHT && weight <= MAX_VALID_WEIGHT);
}

// ✅ SIMPLIFIÉ : Heartbeat basique (2× par cycle seulement)
void sendHeartbeat() {
  digitalWrite(heartbeatPin, HIGH);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  digitalWrite(heartbeatPin, LOW);
}

void ledemergency() {
  digitalWrite(ledPin, HIGH);
  vTaskDelay(400 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, HIGH);
  vTaskDelay(400 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, HIGH);
  vTaskDelay(400 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void ledeupdate() {
  digitalWrite(ledPin, HIGH);
  vTaskDelay(800 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(400 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, HIGH);
  vTaskDelay(400 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void ledwifi() {
  digitalWrite(ledPin, HIGH);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
}

void dot() {
  digitalWrite(ledPin, HIGH);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void dash() {
  digitalWrite(ledPin, HIGH);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void space() {
  vTaskDelay(30 / portTICK_PERIOD_MS);
}

void ledratamuse() {
  dot();
  dash();
  dot();
  space();
  dot();
  dash();
  space();
  dash();
  space();
  dash();
  dash();
  space();
  dot();
  dot();
  dash();
  space();
  dot();
  dot();
  dot();
  space();
  dot();
}

void ledemission() {
  digitalWrite(ledPin, HIGH);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void deepsleep() {
  // ✅ CONSERVÉ : Deep sleep 12h pour économie batterie optimale
  uint32_t TimeToSleep = 21600UL;                          // 6h
  if (batteryPercent >= 75) TimeToSleep = 1800UL;          // 30min
  else if (batteryPercent >= 51) TimeToSleep = 7200UL;     // 2h
  else if (batteryPercent >= 26) TimeToSleep = 21600UL;    // 6h
  else TimeToSleep = 43200UL;                              // 12h

  WiFi.mode(WIFI_OFF);
  uint64_t sleepTimeMicroseconds = (uint64_t)TimeToSleep * uS_TO_S_FACTOR;
  esp_sleep_enable_timer_wakeup(sleepTimeMicroseconds);
  esp_deep_sleep_start();
}

// ---------------- SAFE PREFERENCES (AMÉLIORÉ) ----------------

// ✅ AMÉLIORATION MAJEURE : Gestion correcte des modes RO/RW
bool safePreferencesBegin(bool ro) {
  if (xSemaphoreTake(preferencesSemaphore, pdMS_TO_TICKS(5000)) != pdTRUE) {
    Serial.println("CRITICAL: Preferences semaphore timeout (5s)");
    recordCrash("pref_semaphore_timeout");
    return false;
  }

  // ✅ NOUVEAU : Vérifier si besoin de réouvrir dans un mode différent
  if (preferencesOpen && preferencesReadOnly != ro) {
    Serial.printf("Closing preferences to reopen (was %s, need %s)\n", 
                  preferencesReadOnly ? "RO" : "RW", 
                  ro ? "RO" : "RW");
    preferences.end();
    preferencesOpen = false;
  }

  if (!preferencesOpen) {
    if (!preferences.begin("my-app", ro)) {
      Serial.println("CRITICAL: Preferences begin failed");
      
      // ✅ NOUVEAU : Tentative de récupération si corruption NVS
      nvs_recovery_attempts++;
      if (nvs_recovery_attempts <= 3) {
        Serial.printf("NVS recovery attempt %d/3\n", nvs_recovery_attempts);
        preferences.clear();
        
        // Si échec persistant, effacer complètement la partition
        if (nvs_recovery_attempts >= 3) {
          Serial.println("NVS corruption severe - erasing partition");
          nvs_flash_erase();
          nvs_flash_init();
          nvs_corrupted = true;
        }
        
        // Retry
        if (preferences.begin("my-app", ro)) {
          Serial.println("NVS recovery successful");
          preferencesOpen = true;
          preferencesReadOnly = ro;
          nvs_recovery_attempts = 0;
          return true;
        }
      }
      
      xSemaphoreGive(preferencesSemaphore);
      recordCrash("pref_begin_failed");
      return false;
    }
    preferencesOpen = true;
    preferencesReadOnly = ro;  // ✅ NOUVEAU : Mémoriser le mode
    nvs_recovery_attempts = 0;  // Reset si succès
  }
  
  return true;
}

void safePreferencesEnd() {
  if (preferencesOpen) {
    preferences.end();
    preferencesOpen = false;
    preferencesReadOnly = true;  // Reset
  }
  xSemaphoreGive(preferencesSemaphore);
}

// --- Persistent crash/boot logging helpers (OPTIMISÉ) ---

void recordCrash(const char *type) {
  if (!safePreferencesBegin(false)) return;
  crashCount = preferences.getUInt("crash_count", 0);
  preferences.putUInt("crash_count", crashCount + 1);
  preferences.putString("last_crash_type", String(type));
  preferences.putUInt("last_crash_time", (uint32_t)(esp_timer_get_time() / 1000000ULL));
  safePreferencesEnd();
}

// ✅ OPTIMISÉ : Réduction drastique des écritures
void safeRebootWithReason(const char *reason) {
  if (safePreferencesBegin(false)) {
    preferences.putString("last_reboot_reason", String(reason));
    preferences.putUInt("last_reboot_time", (uint32_t)(esp_timer_get_time() / 1000000ULL));
    safePreferencesEnd();
  }
  esp_restart();
}

// --- FreeRTOS hooks ---
extern "C" void vApplicationMallocFailedHook() {
  Serial.println("FATAL: Malloc failed!");
  recordCrash("malloc_failed");
  safeRebootWithReason("malloc_failed");
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  Serial.printf("FATAL: Stack overflow in task %s\n", pcTaskName ? pcTaskName : "unknown");
  recordCrash("stack_overflow");
  if (safePreferencesBegin(false)) {
    preferences.putString("stack_overflow_task", String(pcTaskName ? pcTaskName : "unknown"));
    safePreferencesEnd();
  }
  safeRebootWithReason("stack_overflow");
}

// calculateApproximatePosition: uses device-specific persistent bias.
void calculateApproximatePosition(float latDeg, float lonDeg, float &approxLat, float &approxLon) {
  if (!safePreferencesBegin(false)) {
    approxLat = latDeg;
    approxLon = lonDeg;
    return;
  }

  float bias_angle = preferences.getFloat("bias_angle", NAN);
  float bias_distance_factor = preferences.getFloat("bias_distance_factor", NAN);

  if (isnan(bias_angle) || isnan(bias_distance_factor)) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    uint32_t seed = (uint32_t)mac[2] << 24 | (uint32_t)mac[3] << 16 | (uint32_t)mac[4] << 8 | (uint32_t)mac[5];
    bias_angle = (float)(seed % 360);
    bias_distance_factor = 0.8f + (float)(seed % 400) / 1000.0f;
    preferences.putFloat("bias_angle", bias_angle);
    preferences.putFloat("bias_distance_factor", bias_distance_factor);
    preferences.putBool("bias_initialized", true);
    Serial.printf("[GPS] New bias stored: angle=%.1f°, factor=%.2f\n", bias_angle, bias_distance_factor);
  }
  safePreferencesEnd();

  float angle = ((float)esp_random() / (float)UINT32_MAX) * 360.0f;
  angle += bias_angle;
  while (angle < 0) angle += 360.0f;
  while (angle >= 360.0f) angle -= 360.0f;

  float distanceMeters = 500.0f + ((float)esp_random() / (float)UINT32_MAX) * 1500.0f;
  distanceMeters *= bias_distance_factor;

  float angleRad = angle * 3.14159265358979323846f / 180.0f;
  float latOffsetDeg = (distanceMeters / 111000.0f) * cos(angleRad);
  float lonOffsetDeg = (distanceMeters / (111000.0f * cos(latDeg * 3.14159265358979323846f / 180.0f))) * sin(angleRad);

  approxLat = latDeg + latOffsetDeg + randomFloat(-0.0005f, 0.0005f);
  approxLon = lonDeg + lonOffsetDeg + randomFloat(-0.0005f, 0.0005f);

  Serial.printf("[GPS] Exact: %.6f, %.6f | Bias angle=%.1f factor=%.2f | Dist=%.0fm angle=%.0f°\n",
                latDeg, lonDeg, bias_angle, bias_distance_factor, distanceMeters, angle);
  Serial.printf("[GPS] Approx: %.6f, %.6f\n", approxLat, approxLon);
}

// ---------- Power & modem helpers ----------
void stabilizePower() {
  PMU.disableDC3();
  vTaskDelay(20 / portTICK_PERIOD_MS);
  float battVoltage = PMU.getBattVoltage();
  if (battVoltage < 3.4f) {
    Serial.println("Batterie critique - Mise en veille");
    uint64_t sleepTime = (uint64_t)12 * 3600ULL * uS_TO_S_FACTOR;
    esp_sleep_enable_timer_wakeup(sleepTime);
    esp_deep_sleep_start();
  }
  PMU.setDC3Voltage(3000);
  PMU.enableDC3();
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

bool isSIMReady() {
  for (int i = 0; i < 3; i++) {
    modem.sendAT("+CPIN?");
    vTaskDelay(200 / portTICK_PERIOD_MS);
    String response;
    unsigned long start = millis();
    while (millis() - start < 5000) {
      if (modem.stream.available()) {
        response = modem.stream.readStringUntil('\n');
        Serial.println("SIM response: " + response);
        if (response.indexOf("READY") != -1) {
          return true;
        } else if (response.indexOf("SIM PIN") != -1 || response.indexOf("NOT INSERTED") != -1) {
          return false;
        }
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return false;
}

bool safeModemStart() {
  size_t freeHeap = esp_get_free_heap_size();
  if (freeHeap < MIN_FREE_HEAP) {
    Serial.printf("Low heap before modem start: %u bytes -> restart\n", (unsigned)freeHeap);
    recordCrash("low_heap_modem");
    safeRebootWithReason("low_heap_modem");
  }

  for (int attempt = 1; attempt <= MODEM_MAX_RETRIES; ++attempt) {
    stabilizePower();
    Serial1.begin(115200, SERIAL_8N1, BOARD_MODEM_RXD_PIN, BOARD_MODEM_TXD_PIN);

    // Séquence PWRKEY: LOW 150ms -> HIGH 1000ms -> LOW 500ms
    pinMode(BOARD_MODEM_PWR_PIN, OUTPUT);
    digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
    vTaskDelay(150 / portTICK_PERIOD_MS);
    digitalWrite(BOARD_MODEM_PWR_PIN, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    Serial.printf("Modem start attempt %d\n", attempt);

    unsigned long start = millis();
    while (millis() - start < 20000UL) {
      if (modem.testAT(2000)) {
        Serial.println("Modem ready (AT OK)");

        // --- Configuration réseau NB-IoT / LTE-M ---
        // Désactiver LED modem
        modem.sendAT("+CNETLIGHT=0");
        modem.waitResponse(5000);

        // Mode avion pour configurer proprement
        modem.sendAT("+CFUN=0");
        modem.waitResponse(10000);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Mode réseau: 1=CAT-M, 2=NB-IoT, 3=CAT-M+NB-IoT
        modem.sendAT("+CMNB=2");
        modem.waitResponse(5000);

        // Bande NB-IoT: B20 (800MHz Bouygues France)
        modem.sendAT("+CBANDCFG=\"NB-IOT\",20");
        modem.waitResponse(5000);

        // APN vide — le réseau attribue automatiquement (ido.net pour Bouygues)
        modem.sendAT("+CGDCONT=1,\"IP\",\"\"");
        modem.waitResponse(5000);

        // Activer infos étendues CEREG (TAC, Cell ID, AcT)
        modem.sendAT("+CEREG=2");
        modem.waitResponse(5000);

        // Erreurs verbose
        modem.sendAT("+CMEE=2");
        modem.waitResponse(5000);

        // Réactiver la radio
        modem.sendAT("+CFUN=1");
        modem.waitResponse(10000);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Sélection opérateur automatique
        modem.sendAT("+COPS=0");
        modem.waitResponse(10000);

        Serial.println("Modem configured (NB-IoT B20 uniquement)");
        return true;
      }
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    modem.poweroff();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return false;
}

bool safeNetworkConnect() {
  if (!isSIMReady()) {
    Serial.println("SIM non prête ou absente -> mise en veille");
    recordCrash("no_sim");
    emergencySleep();
    return false;
  }
  // Lecture ICCID (numéro de carte SIM) — lecture manuelle car getSimCCID()
  // retourne "OK" au lieu de la valeur sur SIM7080G
  modem.sendAT("+CCID");
  simICCID = "";
  unsigned long _iccidStart = millis();
  while (millis() - _iccidStart < 3000) {
    if (modem.stream.available()) {
      String _line = modem.stream.readStringUntil('\n');
      _line.trim();
      // L'ICCID commence toujours par 89 et fait 19-20 chiffres
      if (_line.length() >= 18 && _line.startsWith("89")) {
        simICCID = _line;
        break;
      }
      // Certains modems préfixent avec "+CCID: "
      if (_line.startsWith("+CCID:")) {
        simICCID = _line.substring(6);
        simICCID.trim();
        break;
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  Serial.println("SIM ICCID: " + (simICCID.length() ? simICCID : "non trouvé"));

  // Attente enregistrement réseau via CEREG (NB-IoT/LTE-M)
  // CEREG stat: 0=non enregistré, 1=home, 2=recherche, 3=refusé, 5=roaming
  bool registered = false;
  unsigned long start = millis();

  Serial.println("Attente enregistrement réseau...");
  while (millis() - start < NETWORK_TIMEOUT) {
    esp_task_wdt_reset();

    // Interroger CEREG manuellement pour NB-IoT/LTE-M
    modem.sendAT("+CEREG?");
    String response = "";
    unsigned long cmdStart = millis();
    while (millis() - cmdStart < 3000) {
      if (modem.stream.available()) {
        char c = modem.stream.read();
        response += c;
      }
      if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) break;
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Parser le statut CEREG
    int idx = response.indexOf("+CEREG:");
    if (idx >= 0) {
      int commaIdx = response.indexOf(',', idx);
      if (commaIdx >= 0) {
        char statChar = response.charAt(commaIdx + 1);
        int stat = statChar - '0';

        if (stat == 1 || stat == 5) {
          registered = true;
          Serial.println("Réseau enregistré (CEREG=" + String(stat) + ")");
          break;
        } else if (stat == 3) {
          Serial.println("Réseau refusé (CEREG=3)");
        }
      }
    }

    // Fallback: vérifier aussi via TinyGSM
    int reg = modem.getRegistrationStatus();
    if (reg == REG_OK_HOME || reg == REG_OK_ROAMING) {
      registered = true;
      Serial.println("Réseau enregistré (TinyGSM reg=" + String(reg) + ")");
      break;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  if (!registered) {
    Serial.println("Echec enregistrement réseau (timeout)");
    return false;
  }

  // Vider le buffer série (URC en attente comme +APP PDP, +CEREG, etc.)
  vTaskDelay(500 / portTICK_PERIOD_MS);
  while (modem.stream.available()) modem.stream.read();

  // Vérifier si le PDP est déjà actif (auto-activation à l'attach)
  modem.sendAT("+CNACT?");
  if (modem.waitResponse(5000) == 1) {
    // Vider à nouveau pour être propre
    vTaskDelay(200 / portTICK_PERIOD_MS);
    while (modem.stream.available()) modem.stream.read();
  }

  if (modem.isGprsConnected()) {
    Serial.println("Data déjà actif (PDP auto-activé à l'attach)");
    return true;
  }

  // Sinon, activer manuellement la connexion data (PDP context)
  for (int i = 0; i < 3; i++) {
    esp_task_wdt_reset();

    // D'abord désactiver si dans un état incohérent
    modem.sendAT("+CNACT=0,0");
    modem.waitResponse(5000);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    modem.sendAT("+CNACT=0,1");
    if (modem.waitResponse(15000) == 1) {
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      if (modem.isGprsConnected()) {
        Serial.println("Data connecté (CNACT OK)");
        return true;
      }
    }
    Serial.printf("CNACT tentative %d échouée, retry...\n", i + 1);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  return false;
}

// -----------------------------------------------------------------------
// Callback MQTT — reçoit les commandes FOTA
// -----------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] Message recu sur %s (%u bytes)\n", topic, length);
  if (length > 500 || length == 0) return;

  // Vérifier que c'est bien le topic de commande
  if (String(topic) != mqtt_topic_cmd) return;

  // Parser le JSON
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.printf("[MQTT] JSON invalide: %s\n", err.c_str());
    return;
  }

  const char* action = doc["action"] | "";
  if (strcmp(action, "ota") == 0) {
    fotaUrl          = String(doc["url"]  | "");
    fotaMd5          = String(doc["md5"]  | "");
    fotaExpectedSize = (uint32_t)(doc["size"] | 0);
    if (fotaUrl.length() == 0) {
      Serial.println("[MQTT] URL FOTA manquante");
      return;
    }
    fotaPending = true;
    Serial.printf("[FOTA] Commande OTA recue: url=%s md5=%s size=%u\n",
                  fotaUrl.c_str(), fotaMd5.c_str(), fotaExpectedSize);
  } else if (strcmp(action, "reset") == 0) {
    Serial.println("[MQTT] Commande RESET recue");
    resetPending = true;
  } else if (strcmp(action, "set_config") == 0) {
    int         p   = doc["port"]   | 0;
    const char* srv = doc["server"] | "";
    pendingMqttPort   = (p >= 1 && p <= 65535) ? p : 0;
    pendingMqttServer = String(srv);
    if (pendingMqttPort > 0 || pendingMqttServer.length() > 0) {
      configPending = true;
      Serial.printf("[CMD] set_config demande — port:%d serveur:\"%s\"\n",
                    pendingMqttPort, pendingMqttServer.c_str());
    } else {
      Serial.println("[CMD] set_config: aucun champ valide, ignore");
    }
  } else {
    Serial.printf("[MQTT] Action inconnue: %s\n", action);
  }
}

bool safeMQTTConnect() {
  mqtt.setBufferSize(512);  // nécessaire pour les payloads FOTA (~200 octets)
  mqtt.setServer(cfg.mqtt_server.c_str(), cfg.mqtt_port);
  mqtt.setCallback(mqttCallback);

  unsigned long start = millis();
  while (millis() - start < MQTT_CONNECT_TIMEOUT) {
    esp_task_wdt_reset();

    // Utiliser les credentials générés à partir du MAC eFuse
    if (mqtt.connect(mqtt_client_id.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
      Serial.println("MQTT connected with credentials: " + mqtt_user);
      return true;
    }
    Serial.println("MQTT connection failed, retrying...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  return false;
}

bool safePublish(const String &topic, const char *payload) {
  for (int i = 0; i < 3; i++) {
    esp_task_wdt_reset();

    if (!mqtt.connected()) {
      if (!safeMQTTConnect()) continue;
    }
    if (mqtt.publish(topic.c_str(), payload)) return true;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return false;
}

void turnOffModem() {
  Serial.println("Powering off modem...");
  modem.poweroff();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  Serial.println("Modem off");
}

void emergencySleep() {
  Serial.println("Entering emergency sleep");
  turnOffModem();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  sendHeartbeat();
  
  ledemergency();
  PMU.disableBLDO2();
  PMU.disableDC3();
  PMU.disableDC5();
  deepsleep();
}

// ---------- Web & OTA ----------
// Note: OTA is handled directly in the HTTP upload handler below

void sendCommonHTML(String &body) {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>🐝 Balance " + cfg.deviceName + "</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; ";
  html += "background: linear-gradient(135deg, #FFF8DC 0%, #FFE4B5 100%); ";
  html += "min-height: 100vh; padding: 20px; }";
  
  // Container
  html += ".container { max-width: 800px; margin: 0 auto; background: white; ";
  html += "border-radius: 20px; box-shadow: 0 10px 40px rgba(0,0,0,0.1); overflow: hidden; }";
  
  // Header
  html += ".header { background: linear-gradient(135deg, #FFD700 0%, #FFA500 100%); ";
  html += "padding: 30px 20px; text-align: center; color: #333; }";
  html += ".header h1 { font-size: 2em; margin-bottom: 10px; }";
  html += ".header h1::before { content: '🐝 '; }";
  html += ".header h1::after { content: ' 🍯'; }";
  html += ".version { background: rgba(255,255,255,0.3); display: inline-block; ";
  html += "padding: 5px 15px; border-radius: 20px; font-size: 0.9em; }";
  
  // Content
  html += ".content { padding: 30px 20px; }";
  html += "h2 { color: #FF8C00; margin: 30px 0 15px 0; padding-bottom: 10px; ";
  html += "border-bottom: 2px solid #FFE4B5; }";
  html += "h2::before { content: '🔸 '; }";
  
  // Info cards
  html += ".info-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); ";
  html += "gap: 15px; margin: 20px 0; }";
  html += ".info-card { background: #FFFAF0; padding: 15px; border-radius: 10px; ";
  html += "border-left: 4px solid #FFD700; }";
  html += ".info-card strong { color: #FF8C00; display: block; margin-bottom: 5px; }";
  
  // Links
  html += ".links { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); ";
  html += "gap: 15px; margin: 20px 0; }";
  html += ".link-card { background: linear-gradient(135deg, #FFE4B5 0%, #FFDAB9 100%); ";
  html += "padding: 20px; border-radius: 10px; text-align: center; ";
  html += "text-decoration: none; color: #333; font-weight: 600; ";
  html += "transition: all 0.3s; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }";
  html += ".link-card:hover { transform: translateY(-5px); ";
  html += "box-shadow: 0 6px 12px rgba(0,0,0,0.15); }";
  html += ".link-card::before { font-size: 2em; display: block; margin-bottom: 10px; }";
  html += ".link-balance1::before { content: '⚖️'; }";
  html += ".link-balance2::before { content: '⚖️'; }";
  html += ".link-calib1::before { content: '🔧'; }";
  html += ".link-calib2::before { content: '🔧'; }";
  html += ".link-device::before { content: '📝'; }";
  html += ".link-mqtt::before { content: '📡'; }";
  html += ".link-ota::before { content: '⬆️'; }";
  
  // Forms
  html += "form { background: #FFFAF0; padding: 20px; border-radius: 10px; margin: 20px 0; }";
  html += "input[type=text], input[type=number] { width: 100%; padding: 12px; ";
  html += "margin: 10px 0; border: 2px solid #FFE4B5; border-radius: 8px; ";
  html += "font-size: 1em; transition: border 0.3s; }";
  html += "input[type=text]:focus, input[type=number]:focus { ";
  html += "outline: none; border-color: #FFD700; }";
  
  // Buttons
  html += "input[type=submit], .btn { background: linear-gradient(135deg, #FFD700 0%, #FFA500 100%); ";
  html += "color: #333; padding: 12px 30px; border: none; border-radius: 25px; ";
  html += "font-size: 1em; font-weight: 600; cursor: pointer; ";
  html += "transition: all 0.3s; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }";
  html += "input[type=submit]:hover, .btn:hover { ";
  html += "transform: translateY(-2px); box-shadow: 0 6px 12px rgba(0,0,0,0.2); }";
  html += "input[type=submit]:active, .btn:active { transform: translateY(0); }";
  
  // Reboot button (danger)
  html += ".btn-danger { background: linear-gradient(135deg, #FF6347 0%, #FF4500 100%); ";
  html += "color: white; }";
  
  // Alert
  html += ".alert { background: #FFF3CD; border-left: 4px solid #FFA500; ";
  html += "padding: 15px; border-radius: 8px; margin: 15px 0; }";
  html += ".alert-danger { background: #FFE4E1; border-left-color: #FF4500; }";
  
  // Weight display
  html += ".weight-display { background: linear-gradient(135deg, #FFD700 0%, #FFA500 100%); ";
  html += "padding: 30px; border-radius: 15px; text-align: center; margin: 20px 0; }";
  html += ".weight-value { font-size: 3em; font-weight: bold; color: #333; }";
  html += ".weight-label { font-size: 1.2em; color: #555; margin-top: 10px; }";
  
  // Responsive
  html += "@media (max-width: 600px) {";
  html += ".header h1 { font-size: 1.5em; }";
  html += ".info-grid, .links { grid-template-columns: 1fr; }";
  html += ".weight-value { font-size: 2em; }";
  html += "}";
  
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += body;
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleRoot() {
  // Lire la batterie et la température maintenant
  batteryPercent = PMU.getBatteryPercent();

  // Lire température DS18B20
  int currentTemp = -999;
  sensor.requestTemperatures();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  if (sensor.isConversionComplete()) {
    currentTemp = (int)round(sensor.getTempC());
  }

  String message = "<div class='header'>";
  message += "<h1>" + cfg.deviceName + "</h1>";
  message += "<div class='version'>Firmware v" + String(FIRMWARE_VERSION) + "</div>";
  message += "</div>";
  
  message += "<div class='content'>";
  
  // Diagnostics
  message += "<h2>État du Système</h2>";
  message += "<div class='info-grid'>";
  message += "<div class='info-card'><strong>🔋 Batterie</strong>" + String(batteryPercent) + "%</div>";
  
  // Affichage température
  if (currentTemp != -999) {
    message += "<div class='info-card'><strong>🌡️ Température</strong>" + String(currentTemp) + "°C</div>";
  } else {
    message += "<div class='info-card'><strong>🌡️ Température</strong>Erreur</div>";
  }
  message += "</div>";
  
  // Crashs avec bouton reset
  message += "<div style='background:#FFFAF0;padding:15px;border-radius:10px;border-left:4px solid #FFD700;margin:20px 0;'>";
  message += "<div style='display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px;'>";
  message += "<div><strong style='color:#FF8C00;'>💥 Crashs :</strong> " + String(crashCount);
  if (crashCount > 0 && lastCrashType != "none") {
    message += " <span style='color:#666;font-size:0.9em;'>(" + lastCrashType + ")</span>";
  }
  message += "</div>";
  if (crashCount > 0) {
    message += "<form action='/resetcrash' method='post' style='margin:0;'>";
    message += "<input type='submit' value='🔄 Reset' style='background:linear-gradient(135deg,#FF6347 0%,#FF4500 100%);";
    message += "color:white;padding:8px 20px;border:none;border-radius:20px;font-size:0.9em;font-weight:600;cursor:pointer;'>";
    message += "</form>";
  }
  message += "</div></div>";
  
  if (nvs_corrupted) {
    message += "<div class='alert alert-danger'>⚠️ La mémoire NVS a été corrompue et récupérée</div>";
  }
  
  // Configuration
  message += "<h2>Configuration</h2>";
  message += "<div class='info-grid'>";
  message += "<div class='info-card'><strong>🔑 ID Station</strong>" + deviceId + "</div>";
  message += "<div class='info-card'><strong>📝 Nom</strong>" + cfg.deviceName + "</div>";
  message += "<div class='info-card'><strong>📡 MQTT</strong>" + cfg.mqtt_server + ":" + String(cfg.mqtt_port) + "</div>";
  message += "</div>";

  // Credentials MQTT (lecture seule)
  message += "<h2>🔐 Identifiants MQTT</h2>";
  message += "<div class='info-grid'>";
  message += "<div class='info-card'><strong>👤 Utilisateur</strong><code>" + mqtt_user + "</code></div>";
  message += "<div class='info-card'><strong>🔑 Mot de passe</strong><code style='word-break:break-all;'>" + mqtt_pass + "</code></div>";
  message += "</div>";
  
  // Menu principal
  message += "<h2>Menu Principal</h2>";
  message += "<div class='links'>";
  message += "<a href='/weight1' class='link-card link-balance1'>Balance 1<br>Peser</a>";
  message += "<a href='/weight2' class='link-card link-balance2'>Balance 2<br>Peser</a>";
  message += "<a href='/calibrate1' class='link-card link-calib1'>Calibrer<br>Balance 1</a>";
  message += "<a href='/calibrate2' class='link-card link-calib2'>Calibrer<br>Balance 2</a>";
  message += "<a href='/setDeviceName' class='link-card link-device'>Nom de<br>Station</a>";
  message += "<a href='/setMQTT' class='link-card link-mqtt'>Config<br>MQTT</a>";
  message += "<a href='/update' class='link-card link-ota'>Mise à jour<br>Firmware</a>";
  message += "<a href='/fota' class='link-card link-ota'>FOTA<br>NB-IoT</a>";
  message += "</div>";
  
  // Reboot
  message += "<form action='/reboot' method='post' style='text-align:center;'>";
  message += "<input type='submit' value='🔄 Redémarrer la Station' class='btn btn-danger'>";
  message += "</form>";
  
  message += "</div>";
  sendCommonHTML(message);
}

void handleCalibration1() {
  // --- Étape 2 : poids connu posé → calibration via bibliothèque ---
  if (server.hasArg("known")) {
    float knownWeight = server.arg("known").toFloat();

    if (knownWeight <= 0.0f) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Poids invalide.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    if (!scale1.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 1 non prête. Vérifiez les connexions.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    scale1.calibrate_scale(knownWeight, 20);
    float newFactor = scale1.get_scale();
    int32_t newOffset = scale1.get_offset();

    if (!isValidFactor(newFactor)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Facteur de calibration invalide calculé.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    if (!safePreferencesBegin(false)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur d'écriture des préférences.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    preferences.putFloat("factor1", newFactor);
    preferences.putFloat("offset1", (float)newOffset);
    safePreferencesEnd();

    if (!safePreferencesBegin(true)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur de vérification.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    float readback = preferences.getFloat("factor1", 0.0f);
    safePreferencesEnd();

    if (abs(readback - newFactor) > 0.0001f) {
      Serial.printf("ERROR: Calibration A write verify failed! Wrote %.6f, read %.6f\n",
                    newFactor, readback);
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ La vérification d'écriture a échoué.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    cfg.factor1 = newFactor;
    cfg.offset1 = (float)newOffset;

    String msg = "<div class='header'><h1>✅ Calibration Réussie</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎉 Balance 1 calibrée avec succès !</div>";
    msg += "<div class='info-card'><strong>Facteur de calibration</strong>" + String(newFactor, 6) + "</div>";
    msg += "<div class='info-card'><strong>Offset (zéro)</strong>" + String(newOffset) + "</div>";
    msg += "<div style='display:flex;gap:12px;flex-wrap:wrap;justify-content:center;margin-top:16px;'>";
    msg += "<a href='/weight1' class='btn'>Tester la balance</a>";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    msg += "</div>";
    sendCommonHTML(msg);
  }
  // --- Page initiale : tare automatique sur chargement + saisie du poids ---
  else {
    if (!scale1.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 1 non prête. Vérifiez les connexions.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    scale1.tare(20);

    String form = "<div class='header'><h1>Calibration Balance 1</h1></div>";
    form += "<div class='content'>";
    form += "<div class='alert'>✅ Tare effectuée. Posez maintenant le poids connu sur la balance 1.</div>";
    form += "<form action='/calibrate1' method='get'>";
    form += "<label><strong>Poids connu (grammes) :</strong></label>";
    form += "<input type='number' step='0.01' name='known' required>";
    form += "<br><br><input type='submit' value='🔧 Calibrer'>";
    form += "</form>";
    form += "<br><a href='/calibrate1' class='btn'>Recommencer</a> <a href='/' class='btn'>Retour</a>";
    form += "</div>";
    sendCommonHTML(form);
  }
}

void handleCalibration2() {
  // --- Étape 2 : poids connu posé → calibration via bibliothèque ---
  if (server.hasArg("known")) {
    float knownWeight = server.arg("known").toFloat();

    if (knownWeight <= 0.0f) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Poids invalide.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    if (!scale2.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 2 non prête. Vérifiez les connexions.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    scale2.calibrate_scale(knownWeight, 20);
    float newFactor = scale2.get_scale();
    int32_t newOffset = scale2.get_offset();

    if (!isValidFactor(newFactor)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Facteur de calibration invalide calculé.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    if (!safePreferencesBegin(false)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur d'écriture des préférences.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    preferences.putFloat("factor2", newFactor);
    preferences.putFloat("offset2", (float)newOffset);
    safePreferencesEnd();

    if (!safePreferencesBegin(true)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur de vérification.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    float readback = preferences.getFloat("factor2", 0.0f);
    safePreferencesEnd();

    if (abs(readback - newFactor) > 0.0001f) {
      Serial.printf("ERROR: Calibration B write verify failed! Wrote %.6f, read %.6f\n",
                    newFactor, readback);
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ La vérification d'écriture a échoué.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Recommencer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    cfg.factor2 = newFactor;
    cfg.offset2 = (float)newOffset;

    String msg = "<div class='header'><h1>✅ Calibration Réussie</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎉 Balance 2 calibrée avec succès !</div>";
    msg += "<div class='info-card'><strong>Facteur de calibration</strong>" + String(newFactor, 6) + "</div>";
    msg += "<div class='info-card'><strong>Offset (zéro)</strong>" + String(newOffset) + "</div>";
    msg += "<div style='display:flex;gap:12px;flex-wrap:wrap;justify-content:center;margin-top:16px;'>";
    msg += "<a href='/weight2' class='btn'>Tester la balance</a>";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    msg += "</div>";
    sendCommonHTML(msg);
  }
  // --- Page initiale : tare automatique sur chargement + saisie du poids ---
  else {
    if (!scale2.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 2 non prête. Vérifiez les connexions.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    scale2.tare(20);

    String form = "<div class='header'><h1>Calibration Balance 2</h1></div>";
    form += "<div class='content'>";
    form += "<div class='alert'>✅ Tare effectuée. Posez maintenant le poids connu sur la balance 2.</div>";
    form += "<form action='/calibrate2' method='get'>";
    form += "<label><strong>Poids connu (grammes) :</strong></label>";
    form += "<input type='number' step='0.01' name='known' required>";
    form += "<br><br><input type='submit' value='🔧 Calibrer'>";
    form += "</form>";
    form += "<br><a href='/calibrate2' class='btn'>Recommencer</a> <a href='/' class='btn'>Retour</a>";
    form += "</div>";
    sendCommonHTML(form);
  }
}

void handleWeight1() {
  if (server.hasArg("reset")) {
    if (!scale1.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 1 non prête.";
      msg += "</div><br><a href='/weight1' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    scale1.tare(10);
    int32_t newOffset1 = scale1.get_offset();

    if (!safePreferencesBegin(false)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur sauvegarde NVS.";
      msg += "</div><br><a href='/weight1' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    preferences.putFloat("offset1", (float)newOffset1);
    safePreferencesEnd();

    cfg.offset1 = (float)newOffset1;

    String msg = "<div class='header'><h1>✅ Tare Effectuée</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎯 Balance 1 remise à zéro !</div>";
    msg += "<div class='info-card'><strong>Offset</strong>" + String(newOffset1) + "</div>";
    msg += "<br><a href='/weight1' class='btn'>Peser à nouveau</a> ";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String message = "<div class='header'><h1>⚖️ Balance 1</h1></div>";
    message += "<div class='content'>";
    
    if (scale1.is_ready()) {
      float w1 = scale1.get_units(10);
      int weight = -1;

      if (isValidFloat(w1))
        weight = (int)round(w1);

      if (isValidWeight(weight)) {
        message += "<div class='weight-display'>";
        message += "<div class='weight-value'>" + String(weight) + " g</div>";
        message += "<div class='weight-label'>Poids mesuré</div>";
        message += "</div>";
      } else {
        message += "<div class='alert alert-danger'>⚠️ Poids hors limites ou erreur de lecture</div>";
      }
    } else {
      message += "<div class='alert alert-danger'>⚠️ Balance non prête</div>";
    }
    
    message += "<br>";
    message += "<form action='/weight1' method='get'>";
    message += "<input type='hidden' name='reset' value='1'>";
    message += "<input type='submit' value='🎯 Faire la Tare (Reset)' class='btn'>";
    message += "</form>";
    message += "<br><a href='/weight1' class='btn'>🔄 Rafraîchir</a> ";
    message += "<a href='/' class='btn'>Retour</a>";
    message += "</div>";
    sendCommonHTML(message);
  }
}

void handleWeight2() {
  if (server.hasArg("reset")) {
    if (!scale2.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 2 non prête.";
      msg += "</div><br><a href='/weight2' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    scale2.tare(10);
    int32_t newOffset2 = scale2.get_offset();

    if (!safePreferencesBegin(false)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur sauvegarde NVS.";
      msg += "</div><br><a href='/weight2' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    preferences.putFloat("offset2", (float)newOffset2);
    safePreferencesEnd();

    cfg.offset2 = (float)newOffset2;

    String msg = "<div class='header'><h1>✅ Tare Effectuée</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎯 Balance 2 remise à zéro !</div>";
    msg += "<div class='info-card'><strong>Offset</strong>" + String(newOffset2) + "</div>";
    msg += "<br><a href='/weight2' class='btn'>Peser à nouveau</a> ";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String message = "<div class='header'><h1>⚖️ Balance 2</h1></div>";
    message += "<div class='content'>";
    
    if (scale2.is_ready()) {
      float w2 = scale2.get_units(10);
      int weight = -1;

      if (isValidFloat(w2))
        weight = (int)round(w2);

      if (isValidWeight(weight)) {
        message += "<div class='weight-display'>";
        message += "<div class='weight-value'>" + String(weight) + " g</div>";
        message += "<div class='weight-label'>Poids mesuré</div>";
        message += "</div>";
      } else {
        message += "<div class='alert alert-danger'>⚠️ Poids hors limites ou erreur de lecture</div>";
      }
    } else {
      message += "<div class='alert alert-danger'>⚠️ Balance non prête</div>";
    }
    
    message += "<br>";
    message += "<form action='/weight2' method='get'>";
    message += "<input type='hidden' name='reset' value='1'>";
    message += "<input type='submit' value='🎯 Faire la Tare (Reset)' class='btn'>";
    message += "</form>";
    message += "<br><a href='/weight2' class='btn'>🔄 Rafraîchir</a> ";
    message += "<a href='/' class='btn'>Retour</a>";
    message += "</div>";
    sendCommonHTML(message);
  }
}

void handleSetDeviceName() {
  if (server.hasArg("name")) {
    String newName = server.arg("name");

    if (newName.length() == 0 || newName.length() > 32) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Nom invalide (maximum 32 caractères).";
      msg += "</div><br><a href='/setDeviceName' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    if (!safePreferencesBegin(false)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur d'accès aux préférences.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }

    preferences.putString("deviceName", newName);
    safePreferencesEnd();

    cfg.deviceName = newName;

    String msg = "<div class='header'><h1>✅ Nom Modifié</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎉 Nom de la station changé avec succès !</div>";
    msg += "<div class='info-card'><strong>Nouveau nom</strong>" + newName + "</div>";
    msg += "<div class='alert'>⚠️ Redémarrez la station pour que le WiFi utilise le nouveau nom.</div>";
    msg += "<br><a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String form = "<div class='header'><h1>📝 Nom de la Station</h1></div>";
    form += "<div class='content'>";
    form += "<div class='info-card'><strong>🔑 ID unique (fixe)</strong>" + deviceId + "</div>";
    form += "<div class='alert'>Le nom permet d'identifier facilement votre station. L'ID unique garantit qu'il n'y aura pas de collision de données.</div>";
    form += "<form action='/setDeviceName' method='get'>";
    form += "<label><strong>Nom de la station (max 32 caractères) :</strong></label>";
    form += "<input type='text' name='name' maxlength='32' value='" + cfg.deviceName + "' required>";
    form += "<br><br><input type='submit' value='💾 Enregistrer'>";
    form += "</form>";
    form += "<br><a href='/' class='btn'>Retour</a>";
    form += "</div>";
    sendCommonHTML(form);
  }
}

void handleSetMQTT() {
  if (server.hasArg("server") && server.hasArg("port")) {
    String newServer = server.arg("server");
    int newPort = server.arg("port").toInt();
    
    if (newServer.length() == 0 || newPort < 1 || newPort > 65535) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Configuration MQTT invalide.";
      msg += "</div><br><a href='/setMQTT' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    if (!safePreferencesBegin(false)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur d'accès aux préférences.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    preferences.putString("mqtt_server", newServer);
    preferences.putInt("mqtt_port", newPort);
    safePreferencesEnd();
    
    cfg.mqtt_server = newServer;
    cfg.mqtt_port = newPort;
    
    String msg = "<div class='header'><h1>✅ MQTT Configuré</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎉 Configuration MQTT mise à jour !</div>";
    msg += "<div class='info-grid'>";
    msg += "<div class='info-card'><strong>Serveur</strong>" + newServer + "</div>";
    msg += "<div class='info-card'><strong>Port</strong>" + String(newPort) + "</div>";
    msg += "</div>";
    msg += "<br><a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String form = "<div class='header'><h1>📡 Configuration MQTT</h1></div>";
    form += "<div class='content'>";
    form += "<div class='alert'>Configurez le serveur MQTT pour la transmission des données.</div>";

    // Afficher les credentials MQTT (lecture seule, générés à partir du MAC)
    form += "<h2>🔐 Identifiants MQTT</h2>";
    form += "<div class='alert' style='background:#E8F5E9;border-left-color:#4CAF50;'>";
    form += "Ces identifiants sont générés automatiquement à partir du MAC de l'ESP32.<br>";
    form += "Ils sont uniques et non modifiables. Ajoutez-les à votre serveur MQTT.";
    form += "</div>";
    form += "<div class='info-grid'>";
    form += "<div class='info-card'><strong>👤 Utilisateur</strong><code style='font-size:1.1em;background:#f0f0f0;padding:5px 10px;border-radius:5px;'>" + mqtt_user + "</code></div>";
    form += "<div class='info-card'><strong>🔑 Mot de passe</strong><code style='font-size:1.1em;background:#f0f0f0;padding:5px 10px;border-radius:5px;word-break:break-all;'>" + mqtt_pass + "</code></div>";
    form += "</div>";

    form += "<h2>⚙️ Serveur MQTT</h2>";
    form += "<form action='/setMQTT' method='get'>";
    form += "<label><strong>Serveur MQTT :</strong></label>";
    form += "<input type='text' name='server' value='" + cfg.mqtt_server + "' required>";
    form += "<label><strong>Port :</strong></label>";
    form += "<input type='number' name='port' value='" + String(cfg.mqtt_port) + "' min='1' max='65535' required>";
    form += "<br><br><input type='submit' value='💾 Enregistrer'>";
    form += "</form>";
    form += "<br><a href='/' class='btn'>Retour</a>";
    form += "</div>";
    sendCommonHTML(form);
  }
}

void handleUpdatePage() {
  String content = "<div class='header'><h1>⬆️ Mise à Jour Firmware</h1></div>";
  content += "<div class='content'>";
  content += "<div class='alert'>📦 Version actuelle : <strong>" + String(FIRMWARE_VERSION) + "</strong></div>";
  content += "<div class='alert'>⚠️ Attention : Ne débranchez pas la station pendant la mise à jour !</div>";
  content += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  content += "<label><strong>Sélectionnez le fichier .bin :</strong></label><br><br>";
  content += "<input type='file' name='update' accept='.bin' required>";
  content += "<br><br><input type='submit' value='⬆️ Mettre à Jour'>";
  content += "</form>";
  content += "<br><a href='/' class='btn'>Retour</a>";
  content += "</div>";
  sendCommonHTML(content);
}

void handleFotaPage() {
  // Reset du compteur NVS si demandé
  if (server.hasArg("resetretry") && server.arg("resetretry") == "1") {
    fotaNvsResetRetry();
    Serial.println("[FOTA] Compteur NVS remis a zero depuis interface web");
  }

  const FotaState& state = fotaGetState();
  String content = "<div class='header'><h1>FOTA NB-IoT</h1></div><div class='content'>";
  content += "<div class='alert'>📦 Version actuelle : <strong>" + String(FIRMWARE_VERSION) + "</strong></div>";

  // Dernier essai
  content += "<h2>Dernier essai FOTA</h2><div class='info-grid'>";
  content += "<div class='info-card'><strong>Résultat</strong>"
           + String(fotaResultToString(state.lastResult)) + "</div>";
  content += "<div class='info-card'><strong>URL</strong>"
           + (state.lastUrl.length() > 0 ? state.lastUrl : "(aucun)") + "</div>";
  content += "<div class='info-card'><strong>Taille</strong>"
           + String(state.lastSize) + " octets</div>";
  content += "<div class='info-card'><strong>Progression</strong>"
           + String(state.progressPercent) + "%</div>";
  content += "</div>";

  // Déclenchement via MQTT
  content += "<h2>Déclenchement via MQTT</h2>";
  content += "<div class='alert'>Publiez un message <strong>retained</strong> sur le topic :<br>";
  content += "<code>" + mqtt_topic_cmd + "</code><br><br>";
  content += "Format JSON :<br>";
  content += "<code>{\"action\":\"ota\",\"url\":\"http://serveur/firmware.bin\",\"md5\":\"32hex\",\"size\":1234567}</code><br><br>";
  content += "Le device le recevra au prochain réveil MQTT.</div>";

  // Compteur NVS
  content += "<h2>Compteur tentatives NVS</h2><div class='info-grid'>";
  if (safePreferencesBegin(true)) {
    content += "<div class='info-card'><strong>URL stockée</strong>"
             + preferences.getString("fota_url", "(aucune)") + "</div>";
    content += "<div class='info-card'><strong>Tentatives</strong>"
             + String(preferences.getUChar("fota_retry", 0)) + " / " + String(FOTA_MAX_RETRIES) + "</div>";
    safePreferencesEnd();
  } else {
    content += "<div class='info-card'><strong>NVS</strong>Indisponible</div>";
  }
  content += "</div>";

  // Reset compteur
  content += "<form action='/fota' method='get'>";
  content += "<input type='hidden' name='resetretry' value='1'>";
  content += "<input type='submit' value='🔄 Remettre compteur à zéro'>";
  content += "</form>";

  content += "<br><div class='alert'>⚠️ La mise à jour est déclenchée uniquement via MQTT "
             "en mode normal (NB-IoT actif), pas depuis cette page.</div>";
  content += "<br><a href='/' class='btn'>Retour</a></div>";
  sendCommonHTML(content);
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting");
  vTaskDelay(200 / portTICK_PERIOD_MS);
  esp_restart();
}

void handleResetCrash() {
  if (!safePreferencesBegin(false)) {
    String msg = "<div class='header'><h1>Erreur</h1></div>";
    msg += "<div class='content'><div class='alert alert-danger'>";
    msg += "⚠️ Erreur d'accès aux préférences.";
    msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
    sendCommonHTML(msg);
    return;
  }
  
  preferences.putUInt("crash_count", 0);
  preferences.putString("last_crash_type", "none");
  safePreferencesEnd();
  
  crashCount = 0;
  lastCrashType = "none";
  
  String msg = "<div class='header'><h1>✅ Compteur Réinitialisé</h1></div>";
  msg += "<div class='content'>";
  msg += "<div class='alert'>🎉 Le compteur de crashs a été remis à zéro !</div>";
  msg += "<br><a href='/' class='btn'>Retour au menu</a>";
  msg += "</div>";
  sendCommonHTML(msg);
}

// ---------- GPS flow ----------
void gpsget() {
  Serial.println("Switching off GPRS for GPS...");
  modem.sendAT("+CNACT=0,0");
  modem.waitResponse(3000);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  modem.disableGPS();
  vTaskDelay(200 / portTICK_PERIOD_MS);

  Serial.println("Config GPS...");
  modem.sendAT("+CGNSMOD=1,0,0,1,0");
  modem.waitResponse(2000);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  modem.sendAT("+SGNSCMD=2,1000,0,0");
  modem.waitResponse(2000);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  modem.sendAT("+SGNSCMD=0");
  modem.waitResponse(2000);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  Serial.println("Enable GPS...");
  if (!modem.enableGPS()) {
    Serial.println("Enable GPS failed");
    emergencySleep();
  }

  unsigned long start = millis();
  float latf = 0, lonf = 0;
  while (millis() - start < GPS_TIMEOUT) {
    esp_task_wdt_reset();
    
    if (modem.getGPS(&latf, &lonf, nullptr, nullptr, nullptr, nullptr, nullptr,
                     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
      if (latf != 0.0f && lonf != 0.0f) {
        Serial.printf("GPS fix: %.6f, %.6f\n", latf, lonf);
        lat_e6 = (long)round(latf * 1e6);
        lon_e6 = (long)round(lonf * 1e6);
        gpsDataAvailable = true;
        break;
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  turnOffModem();
  vTaskDelay(500 / portTICK_PERIOD_MS);

  if (!safeModemStart()) {
    Serial.println("Modem restart failed after GPS");
    emergencySleep();
  }
  if (!safeNetworkConnect()) {
    Serial.println("Network reconnect failed after GPS");
    emergencySleep();
  }
}

// ---------- Tasks & watchdog integration ----------

void appTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  Serial.println("AppTask: en attente de la fin de l'initialisation...");
  if (xSemaphoreTake(setupCompleteSemaphore, portMAX_DELAY) == pdTRUE) {
    Serial.println("AppTask: initialisation terminée, démarrage de la boucle principale");
  } else {
    Serial.println("AppTask: erreur lors de l'attente du sémaphore");
    emergencySleep();
  }

  if (digitalRead(wifiPin) == LOW) {
    // Mode AP pour configuration
    while (true) {
      server.handleClient();
      ledwifi();
      esp_task_wdt_reset();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  } else {
    // Mode normal
    while (true) {
      mainLoop();
      // On ne devrait JAMAIS arriver ici car mainLoop() appelle deepsleep()
      Serial.println("CRITICAL: mainLoop returned!");
      recordCrash("mainloop_returned");
      esp_task_wdt_reset();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

// ---------- Setup variations ----------
void setupAPModeInit() {
  Serial.begin(115200);
  uint32_t serialStart = millis();
  while (!Serial && (millis() - serialStart < 5000)) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) emergencySleep();
  PMU.setDC3Voltage(3000);
  PMU.enableDC3();
  PMU.setBLDO2Voltage(3300);
  PMU.enableBLDO2();
  PMU.setDC5Voltage(3300);
  PMU.enableDC5();
  PMU.enableBattVoltageMeasure();
  PMU.enableSystemVoltageMeasure();
  PMU.disableTSPinMeasure();

  pinMode(ledPin, OUTPUT);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, HIGH);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  digitalWrite(ledPin, LOW);

  scale1.begin(dataPin1, clockPin1);
  scale2.begin(dataPin2, clockPin2);

  // ✅ ReadOnly suffisant - Charger TOUTE la configuration
  if (!safePreferencesBegin(true)) {
    Serial.println("WARNING: Failed to read preferences in AP mode, using defaults");
    cfg.deviceName = "F7";
    cfg.mqtt_server = "ratamuse.hopto.org";
    cfg.mqtt_port = 1885;
    cfg.factor1 = 1.0f;
    cfg.factor2 = 1.0f;
    cfg.offset1 = 0.0f;
    cfg.offset2 = 0.0f;
  } else {
    cfg.deviceName = preferences.getString("deviceName", "F7");
    cfg.mqtt_server = preferences.getString("mqtt_server", "ratamuse.hopto.org");
    cfg.mqtt_port = preferences.getInt("mqtt_port", 1885);

    // ✅ AJOUT : Charger les facteurs et offsets de calibration
    float f1 = preferences.getFloat("factor1", 1.0);
    float f2 = preferences.getFloat("factor2", 1.0);
    float o1 = preferences.getFloat("offset1", 0.0);
    float o2 = preferences.getFloat("offset2", 0.0);

    cfg.factor1 = isValidFactor(f1) ? f1 : 1.0f;
    cfg.factor2 = isValidFactor(f2) ? f2 : 1.0f;
    cfg.offset1 = isValidFloat(o1) ? o1 : 0.0f;
    cfg.offset2 = isValidFloat(o2) ? o2 : 0.0f;

    safePreferencesEnd();
  }

  // ✅ AJOUT : Configurer les balances avec les facteurs et offsets de calibration
  scale1.set_scale(cfg.factor1);
  scale1.set_offset((int32_t)cfg.offset1);
  scale2.set_scale(cfg.factor2);
  scale2.set_offset((int32_t)cfg.offset2);

  // Générer l'ID unique du device (basé sur eFuse MAC)
  deviceId = getDeviceId();
  Serial.println("Device ID: " + deviceId);

  // Générer les credentials MQTT uniques (basés sur eFuse MAC)
  generateMQTTCredentials(mqtt_user, mqtt_pass);
  Serial.println("MQTT User: " + mqtt_user);
  Serial.println("MQTT Pass: " + mqtt_pass);

  Serial.println("=== Configuration AP Mode ===");
  Serial.printf("Factor1: %.6f, Offset1: %.6f\n", cfg.factor1, cfg.offset1);
  Serial.printf("Factor2: %.6f, Offset2: %.6f\n", cfg.factor2, cfg.offset2);

  // Utiliser deviceName pour le SSID WiFi (plus lisible)
  String ssid = String(ssidPrefix) + cfg.deviceName;
  WiFi.softAP(ssid.c_str(), password1);
  Serial.println("SoftAP created: " + ssid);
  Serial.println("Password: " + String(password1));

  server.on("/", handleRoot);
  server.on("/calibrate1", handleCalibration1);
  server.on("/calibrate2", handleCalibration2);
  server.on("/weight1", handleWeight1);
  server.on("/weight2", handleWeight2);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/resetcrash", HTTP_POST, handleResetCrash);
  server.on("/setDeviceName", handleSetDeviceName);
  server.on("/setMQTT", handleSetMQTT);

  server.on("/update", HTTP_POST, 
    []() {
        // Ne rien envoyer ici
    },
    []() {
        HTTPUpload& upload = server.upload();
        
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("OTA: Début - %s\n", upload.filename.c_str());
            server.setContentLength(CONTENT_LENGTH_UNKNOWN);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
                if (server.client().connected()) {
                    server.send(500, "text/plain", "Update start failed");
                }
            }
            
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
                if (server.client().connected()) {
                    server.send(500, "text/plain", "Write failed");
                }
            }
            
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.println("OTA Réussi - Redémarrage...");
                if (server.client().connected()) {
                    server.sendHeader("Connection", "close");
                    server.sendHeader("Access-Control-Allow-Origin", "*");
                    server.send(200, "text/plain", "Update successful. Rebooting...");
                    delay(100);
                    server.client().stop();
                }
                
                delay(500);
                Serial.println("Préparation du redémarrage...");
                turnOffModem();
                ledeupdate();
                
                PMU.disableBLDO2();
                PMU.disableDC3();
                PMU.disableDC5();
                
                server.stop();
                WiFi.softAPdisconnect(true);
                delay(100);
                esp_restart();
                
            } else {
                Update.printError(Serial);
                if (server.client().connected()) {
                    server.send(500, "text/plain", "Update failed");
                }
            }
            
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
            Serial.println("OTA annulé");
            Update.end(false);
            if (server.client().connected()) {
                server.send(500, "text/plain", "Update aborted");
            }
        }
    }
  );

  server.on("/update", HTTP_GET, handleUpdatePage);
  server.on("/fota",   HTTP_GET, handleFotaPage);   // v1.0.13 FOTA NB-IoT

  server.begin();
  setupComplete = true;
  xSemaphoreGive(setupCompleteSemaphore);
  Serial.println("Initialisation AP terminée, sémaphore libéré");
}

// ✅ Fonction pour charger config avec validation complète
void loadConfigSafe() {
  if (!safePreferencesBegin(true)) {  // ✅ ReadOnly suffisant
    Serial.println("CRITICAL: Cannot access preferences, using defaults");
    cfg.factor1 = 1.0f;
    cfg.factor2 = 1.0f;
    cfg.offset1 = 0.0f;
    cfg.offset2 = 0.0f;
    cfg.deviceName = "F7";
    cfg.mqtt_server = "ratamuse.hopto.org";
    cfg.mqtt_port = 1885;
    return;
  }

  float f1 = preferences.getFloat("factor1", 1.0);
  float f2 = preferences.getFloat("factor2", 1.0);
  float o1 = preferences.getFloat("offset1", 0.0);
  float o2 = preferences.getFloat("offset2", 0.0);

  // Validation stricte
  cfg.factor1 = isValidFactor(f1) ? f1 : 1.0f;
  cfg.factor2 = isValidFactor(f2) ? f2 : 1.0f;
  cfg.offset1 = isValidFloat(o1) ? o1 : 0.0f;
  cfg.offset2 = isValidFloat(o2) ? o2 : 0.0f;

  if (!isValidFactor(f1) || !isValidFactor(f2)) {
    Serial.println("WARNING: Invalid calibration factors detected, using defaults");
    recordCrash("invalid_calibration");
  }

  cfg.deviceName = preferences.getString("deviceName", "F7");
  if (cfg.deviceName.length() == 0 || cfg.deviceName.length() > 32) {
    cfg.deviceName = "F7";
  }

  cfg.mqtt_server = preferences.getString("mqtt_server", "ratamuse.hopto.org");
  cfg.mqtt_port = preferences.getInt("mqtt_port", 1885);
  
  if (cfg.mqtt_port < 1 || cfg.mqtt_port > 65535) {
    cfg.mqtt_port = 1885;
  }

  safePreferencesEnd();

  Serial.println("=== Configuration Loaded ===");
  Serial.println("Factor 1: " + String(cfg.factor1, 6));
  Serial.println("Factor 2: " + String(cfg.factor2, 6));
  Serial.println("Offset 1: " + String(cfg.offset1, 6));
  Serial.println("Offset 2: " + String(cfg.offset2, 6));
  Serial.println("Device: " + cfg.deviceName);
  Serial.println("MQTT: " + cfg.mqtt_server + ":" + String(cfg.mqtt_port));
}

void setupNormalInit() {
  Serial.begin(115200);
  while (!Serial) { vTaskDelay(10 / portTICK_PERIOD_MS); }

  if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) emergencySleep();
  PMU.setDC3Voltage(3000);
  PMU.enableDC3();
  PMU.setBLDO2Voltage(3300);
  PMU.enableBLDO2();
  PMU.setDC5Voltage(3300);
  PMU.enableDC5();
  PMU.enableBattVoltageMeasure();
  PMU.enableSystemVoltageMeasure();
  PMU.disableTSPinMeasure();

  pinMode(ledPin, OUTPUT);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  ledratamuse();

  // Générer l'ID unique du device (basé sur eFuse MAC)
  deviceId = getDeviceId();
  mqtt_client_id = "device-" + deviceId;
  Serial.println("Device ID: " + deviceId);

  // Générer les credentials MQTT uniques (basés sur eFuse MAC)
  generateMQTTCredentials(mqtt_user, mqtt_pass);
  Serial.println("MQTT User: " + mqtt_user);
  Serial.println("MQTT Pass: " + mqtt_pass);

  if (!safeModemStart()) {
    Serial.println("Modem failed init");
    emergencySleep();
  }

  loadConfigSafe();

  // Topics MQTT : deviceId (unique) + deviceName (lisible)
  // Format: A1B2C3D4E5F6/F7/<>/temp1/<>/data/0
  mqtt_topic_temp1 = deviceId + "/" + cfg.deviceName + "/<>/temp1/<>/data/0";
  mqtt_topic_scaleA = deviceId + "/" + cfg.deviceName + "/<>/scaleA/<>/data/0";
  mqtt_topic_scaleB = deviceId + "/" + cfg.deviceName + "/<>/scaleB/<>/data/0";
  mqtt_topic_battery = deviceId + "/" + cfg.deviceName + "/<>/battery/<>/data/0";
  mqtt_topic_signal = deviceId + "/" + cfg.deviceName + "/<>/signal/<>/data/0";
  mqtt_topic_position = deviceId + "/" + cfg.deviceName + "/<>/position/<>/data/0";
  mqtt_topic_position_approx = deviceId + "/" + cfg.deviceName + "/<>/posap/<>/data/0";
  mqtt_topic_diagnostics = deviceId + "/" + cfg.deviceName + "/<>/diagnostics/<>/data/0";
  mqtt_topic_cmd         = deviceId + "/cmd";  // Commandes FOTA

  scale1.begin(dataPin1, clockPin1);
  scale1.set_scale(cfg.factor1);
  scale1.set_offset((int32_t)cfg.offset1);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  scale2.begin(dataPin2, clockPin2);
  scale2.set_scale(cfg.factor2);
  scale2.set_offset((int32_t)cfg.offset2);
  vTaskDelay(50 / portTICK_PERIOD_MS);

  sensor.begin();
  vTaskDelay(50 / portTICK_PERIOD_MS);
  sensor.setResolution(10);
  vTaskDelay(50 / portTICK_PERIOD_MS);

  setupComplete = true;
  xSemaphoreGive(setupCompleteSemaphore);
  Serial.println("Initialisation terminée, sémaphore libéré");
}

// ---------- Main data collection & publish ----------
void mainLoop() {
  // ✅ Checkpoint 0 : Début du cycle
  esp_task_wdt_reset();
  Serial.println("\n=== Début Cycle ===");

  size_t freeHeap = esp_get_free_heap_size();
  Serial.printf("Free heap: %u bytes\n", (unsigned)freeHeap);
  
  if (freeHeap < MIN_FREE_HEAP) {
    Serial.printf("CRITICAL: Low heap -> restart\n");
    recordCrash("low_heap_mainloop");
    safeRebootWithReason("low_heap_mainloop");
  }

  int temp1 = -1;
  int scaleA = -1, scaleB = -1;

  // Lecture Scale A
  unsigned long tstart = millis();
  while (!scale1.is_ready()) {
    if (millis() - tstart > 2000) {
      Serial.println("Scale A not ready after 2s");
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  
  if (scale1.is_ready()) {
    float wA = scale1.get_units(10);
    if (isValidFloat(wA)) {
      scaleA = (int)round(wA);
      if (!isValidWeight(scaleA)) {
        Serial.printf("WARNING: Scale A weight out of range: %d\n", scaleA);
        recordCrash("scale_A_invalid");
        scaleA = -1;
      }
    }
  }

  // Lecture Scale B
  tstart = millis();
  while (!scale2.is_ready()) {
    if (millis() - tstart > 2000) {
      Serial.println("Scale B not ready after 2s");
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  if (scale2.is_ready()) {
    float wB = scale2.get_units(10);
    if (isValidFloat(wB)) {
      scaleB = (int)round(wB);
      if (!isValidWeight(scaleB)) {
        Serial.printf("WARNING: Scale B weight out of range: %d\n", scaleB);
        recordCrash("scale_B_invalid");
        scaleB = -1;
      }
    }
  }

  sensor.requestTemperatures();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  if (sensor.isConversionComplete()) {
    temp1 = (int)round(sensor.getTempC());
  } else {
    Serial.println("Temp sensor error");
  }

  Serial.printf("Capteurs - Temp:%d°C ScaleA:%dg ScaleB:%dg\n", temp1, scaleA, scaleB);

  batteryPercent = PMU.getBatteryPercent();
  Serial.printf("Battery: %d%%\n", batteryPercent);

  // ✅ Checkpoint 1 : Après capteurs
  esp_task_wdt_reset();

  // Connexion réseau
  if (!safeNetworkConnect()) {
    Serial.println("Network connect failed");
    emergencySleep();
  }

  signalStrength = modem.getSignalQuality();
  Serial.printf("Signal: %d\n", signalStrength);

  // ✅ Checkpoint 2 : Après réseau
  esp_task_wdt_reset();

  // Connexion MQTT
  if (!safeMQTTConnect()) {
    Serial.println("MQTT connect failed");
    emergencySleep();
  }

  // Publications MQTT
  char temp1_str[10];
  itoa(temp1, temp1_str, 10);
  safePublish(mqtt_topic_temp1, temp1_str);

  char scaleA_str[10];
  itoa(scaleA, scaleA_str, 10);
  safePublish(mqtt_topic_scaleA, scaleA_str);

  char scaleB_str[10];
  itoa(scaleB, scaleB_str, 10);
  safePublish(mqtt_topic_scaleB, scaleB_str);

  char bat_str[10];
  itoa(batteryPercent, bat_str, 10);
  safePublish(mqtt_topic_battery, bat_str);

  char signal_str[10];
  itoa(signalStrength, signal_str, 10);
  safePublish(mqtt_topic_signal, signal_str);

  // Publication diagnostics
  StaticJsonDocument<384> diagDoc;
  diagDoc["firmware"] = FIRMWARE_VERSION;
  diagDoc["iccid"] = simICCID;
  diagDoc["crash_count"] = crashCount;
  diagDoc["last_crash_type"] = lastCrashType;
  diagDoc["nvs_corrupted"] = nvs_corrupted;
  char diagBuffer[384];
  serializeJson(diagDoc, diagBuffer);
  safePublish(mqtt_topic_diagnostics, diagBuffer);

  // ✅ Checkpoint 3 : Après publications
  esp_task_wdt_reset();

  // ---- Vérification commande FOTA (topic retained) ----
  mqtt.subscribe(mqtt_topic_cmd.c_str());
  unsigned long fotaWait = millis();
  while (millis() - fotaWait < 3000) {   // 3s pour recevoir le retained
    mqtt.loop();
    esp_task_wdt_reset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (fotaPending || resetPending) break;
  }
  mqtt.unsubscribe(mqtt_topic_cmd.c_str());

  // ---- Commande RESET : remet à zéro crash_count et fota_retry en NVS ----
  if (resetPending) {
    resetPending = false;
    Serial.println("[CMD] === Reset compteurs NVS ===");

    // Remise à zéro crash_count
    if (safePreferencesBegin(false)) {
      preferences.putUInt("crash_count", 0);
      preferences.putString("last_crash_type", "none");
      safePreferencesEnd();
      crashCount = 0;
      lastCrashType = "none";
      Serial.println("[CMD] crash_count remis a 0");
    }

    // Remise à zéro compteur tentatives FOTA
    fotaNvsResetRetry();
    Serial.println("[CMD] fota_retry remis a 0");

    // Supprimer le retained /cmd pour ne pas re-déclencher
    mqtt.publish(mqtt_topic_cmd.c_str(), (const uint8_t*)"", 0, true);

    String ackJson = "{\"status\":\"counters_reset\",\"firmware\":\"" + String(FIRMWARE_VERSION) + "\"}";
    mqtt.publish((mqtt_topic_cmd + "/ack").c_str(),
                 (uint8_t*)ackJson.c_str(), ackJson.length(), false);
    vTaskDelay(300 / portTICK_PERIOD_MS);
    Serial.println("[CMD] Compteurs remis a zero, cycle continue normalement");
  }

  // ---- Commande SET_CONFIG : change serveur et/ou port MQTT en NVS puis redémarre ----
  if (configPending) {
    configPending = false;
    Serial.println("[CMD] === set_config : mise a jour config MQTT ===");

    if (safePreferencesBegin(false)) {
      if (pendingMqttPort >= 1 && pendingMqttPort <= 65535) {
        preferences.putInt("mqtt_port", pendingMqttPort);
        cfg.mqtt_port = pendingMqttPort;
        Serial.printf("[CMD] Nouveau port %d sauvegarde\n", pendingMqttPort);
      }
      if (pendingMqttServer.length() > 0) {
        preferences.putString("mqtt_server", pendingMqttServer);
        cfg.mqtt_server = pendingMqttServer;
        Serial.printf("[CMD] Nouveau serveur \"%s\" sauvegarde\n", pendingMqttServer.c_str());
      }
      safePreferencesEnd();
    } else {
      Serial.println("[CMD] ERREUR: impossible d'ecrire en NVS, config inchangee");
    }

    // Effacer le retained /cmd pour ne pas re-declencher apres reboot
    mqtt.publish(mqtt_topic_cmd.c_str(), (const uint8_t*)"", 0, true);

    String ackJson = "{\"action\":\"set_config\",\"status\":\"ok\","
                     "\"port\":"    + String(cfg.mqtt_port) +
                     ",\"server\":\"" + cfg.mqtt_server + "\""
                     ",\"firmware\":\"" + String(FIRMWARE_VERSION) + "\"}";
    mqtt.publish((mqtt_topic_cmd + "/ack").c_str(),
                 (uint8_t*)ackJson.c_str(), ackJson.length(), false);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.println("[CMD] Redemarrage pour appliquer la nouvelle config...");
    safeRebootWithReason("set_config");
  }

  if (fotaPending) {
    fotaPending = false;
    Serial.println("[FOTA] === Commande OTA recue ===");

    uint8_t retries = fotaNvsGetRetryCount(fotaUrl);
    Serial.printf("[FOTA] Tentative %u / %d\n", retries + 1, FOTA_MAX_RETRIES);

    if (retries >= FOTA_MAX_RETRIES) {
      Serial.printf("[FOTA] Max retries atteint (%d)\n", FOTA_MAX_RETRIES);
      String ackJson = "{\"status\":\"max_retries\",\"url\":\"" + fotaUrl + "\"}";
      mqtt.publish((mqtt_topic_cmd + "/ack").c_str(), ackJson.c_str());
      // Supprimer le retained pour éviter une boucle infinie
      mqtt.publish(mqtt_topic_cmd.c_str(), (const uint8_t*)"", 0, true);
      mqtt.setCallback(NULL);
    } else {
      fotaNvsIncrementRetry(fotaUrl);

      // Fermer MQTT + TCP mux0 avant fotaExecute
      mqtt.disconnect();
      client.stop();
      mqtt.setCallback(NULL);
      vTaskDelay(500 / portTICK_PERIOD_MS);

      // Purger le buffer série du modem
      while (modem.stream.available()) modem.stream.read();
      vTaskDelay(200 / portTICK_PERIOD_MS);

      FotaResult result = fotaExecute(fotaUrl, fotaMd5, fotaExpectedSize);
      Serial.printf("[FOTA] Résultat : %s\n", fotaResultToString(result));

      // Purger avant reconnexion MQTT
      while (modem.stream.available()) modem.stream.read();
      vTaskDelay(300 / portTICK_PERIOD_MS);

      // Reconnecter MQTT pour publier l'ack
      if (safeMQTTConnect()) {
        String ackJson = "{\"status\":\"" + String(fotaResultToString(result))
                       + "\",\"url\":\"" + fotaUrl
                       + "\",\"firmware\":\"" + String(FIRMWARE_VERSION) + "\"}";
        mqtt.publish((mqtt_topic_cmd + "/ack").c_str(),
                     (uint8_t*)ackJson.c_str(), ackJson.length(), false);

        if (result == FOTA_OK) {
          // Supprimer le retained pour ne pas re-déclencher au prochain boot
          mqtt.publish(mqtt_topic_cmd.c_str(), (const uint8_t*)"", 0, true);
          fotaNvsResetRetry();
          vTaskDelay(500 / portTICK_PERIOD_MS);
          mqtt.disconnect();
          vTaskDelay(300 / portTICK_PERIOD_MS);
          Serial.println("[FOTA] Succes — redémarrage...");
          esp_restart();
        }
      }
    }
  } else {
    mqtt.setCallback(NULL);
  }
  // ---- Fin du bloc FOTA ----
  esp_task_wdt_reset();

  // GPS (si plage horaire)
  int hour = 0, min = 0;
  if (modem.getNetworkTime(nullptr, nullptr, nullptr, &hour, &min, nullptr, nullptr)) {
    if ((hour == 22 && min >= 0 && min <= 35) || 
        (hour == 10 && min >= 0 && min <= 35 && batteryPercent > 80)) {
      
      Serial.println("GPS window - starting acquisition");
      gpsget();
      
      // ✅ Checkpoint 4 : Après GPS
      esp_task_wdt_reset();
      
      if (gpsDataAvailable) {
        float latf = lat_e6 / 1e6f;
        float lonf = lon_e6 / 1e6f;
        
        StaticJsonDocument<200> exactDoc;
        exactDoc["latitude"] = latf;
        exactDoc["longitude"] = lonf;
        char exactBuffer[200];
        serializeJson(exactDoc, exactBuffer);
        safePublish(mqtt_topic_position, exactBuffer);

        float approxLat, approxLon;
        calculateApproximatePosition(latf, lonf, approxLat, approxLon);
        approxLat = roundf(approxLat * 10000.0f) / 10000.0f;
        approxLon = roundf(approxLon * 10000.0f) / 10000.0f;

        StaticJsonDocument<200> approxDoc;
        approxDoc["latitude"] = approxLat;
        approxDoc["longitude"] = approxLon;
        char approxBuffer[200];
        serializeJson(approxDoc, approxBuffer);
        safePublish(mqtt_topic_position_approx, approxBuffer);
      }
    }
  }

  Serial.println("Going to deep sleep (normal cycle)");
  turnOffModem();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  // ✅ Heartbeat final avant sleep
  sendHeartbeat();
  
  ledemission();
  PMU.disableBLDO2();
  PMU.disableDC3();
  PMU.disableDC5();
  
  deepsleep();
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// ---------- Setup & loop ----------
void setup() {
  srand(esp_random());
  pinMode(wifiPin, INPUT_PULLUP);
  pinMode(heartbeatPin, OUTPUT);
  digitalWrite(heartbeatPin, LOW);

  // ✅ CRITIQUE : Heartbeat IMMÉDIAT au réveil pour reset ATtiny
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sendHeartbeat();
  Serial.begin(115200);
  Serial.println("\n=== Boot ===");
  Serial.println("Heartbeat envoyé au réveil");

  preferencesSemaphore = xSemaphoreCreateMutex();
  if (!preferencesSemaphore) {
    Serial.println("FATAL: Preferences mutex failed");
    emergencySleep();
  }

  setupCompleteSemaphore = xSemaphoreCreateBinary();
  if (!setupCompleteSemaphore) {
    Serial.println("FATAL: setupCompleteSemaphore creation failed");
    emergencySleep();
  }
  
  // Charger crash_count et last_crash_type pour diagnostics
  if (safePreferencesBegin(true)) {
    crashCount = preferences.getUInt("crash_count", 0);
    lastCrashType = preferences.getString("last_crash_type", "none");
    safePreferencesEnd();
  }

  // ✅ TWDT à 6 min
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = TWDT_TIMEOUT_MS,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);

  xTaskCreatePinnedToCore(appTask, "appTask", 20000, NULL, 1, &appTaskHandle, 0);

  if (appTaskHandle) esp_task_wdt_add(appTaskHandle);

  if (digitalRead(wifiPin) == LOW) setupAPModeInit();
  else setupNormalInit();
}
