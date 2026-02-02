/*
  Balance_Nb_Iot_web_v4_TTGO_SIM7080G_mqtt_gps_V1.0.10_FINAL.ino
  
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
// #define DUMP_AT_COMMANDS

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

// ---------- CONSTANTS ----------
const char *ssidPrefix = "balance";
const char *password1 = "123456789";  // ⚠️ CHANGEZ AVANT DÉPLOIEMENT
const char *FIRMWARE_VERSION = "1.0.10";
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

const int ledPin = 1;
const int wifiPin = 47;
const int heartbeatPin = 10;

// Watchdog & tasks
TaskHandle_t appTaskHandle = NULL;

// MQTT / prefs
const char *mqtt_user = "";
const char *mqtt_pass = "";
String mqtt_client_id = "";
String mqtt_topic_temp1, mqtt_topic_scaleA, mqtt_topic_scaleB;
String mqtt_topic_battery, mqtt_topic_signal, mqtt_topic_position, mqtt_topic_position_approx;
String mqtt_topic_diagnostics;

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

  unsigned long start = millis();
  while (millis() - start < NETWORK_TIMEOUT) {
    esp_task_wdt_reset();

    int reg = modem.getRegistrationStatus();
    if (reg == REG_OK_HOME || reg == REG_OK_ROAMING) {
      break;
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  for (int i = 0; i < 3; i++) {
    esp_task_wdt_reset();
    
    modem.sendAT("+CNACT=0,1");
    if (modem.waitResponse(15000) == 1 && modem.isGprsConnected()) {
      return true;
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  return false;
}

bool safeMQTTConnect() {
  mqtt.setServer(cfg.mqtt_server.c_str(), cfg.mqtt_port);

  unsigned long start = millis();
  while (millis() - start < MQTT_CONNECT_TIMEOUT) {
    esp_task_wdt_reset();

    if (mqtt.connect(mqtt_client_id.c_str(), mqtt_user, mqtt_pass)) {
      return true;
    }
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
  message += "<div class='info-card'><strong>📝 Nom</strong>" + cfg.deviceName + "</div>";
  message += "<div class='info-card'><strong>📡 MQTT</strong>" + cfg.mqtt_server + ":" + String(cfg.mqtt_port) + "</div>";
  message += "</div>";
  
  // Menu principal
  message += "<h2>Menu Principal</h2>";
  message += "<div class='links'>";
  message += "<a href='/weight1' class='link-card link-balance1'>Balance 1<br>Peser</a>";
  message += "<a href='/weight2' class='link-card link-balance2'>Balance 2<br>Peser</a>";
  message += "<a href='/calibrate1' class='link-card link-calib1'>Calibrer<br>Balance 1</a>";
  message += "<a href='/calibrate2' class='link-card link-calib2'>Calibrer<br>Balance 2</a>";
  message += "<a href='/setDeviceName' class='link-card link-device'>Nom du<br>Device</a>";
  message += "<a href='/setMQTT' class='link-card link-mqtt'>Config<br>MQTT</a>";
  message += "<a href='/update' class='link-card link-ota'>Mise à jour<br>Firmware</a>";
  message += "</div>";
  
  // Reboot
  message += "<form action='/reboot' method='post' style='text-align:center;'>";
  message += "<input type='submit' value='🔄 Redémarrer le Device' class='btn btn-danger'>";
  message += "</form>";
  
  message += "</div>";
  sendCommonHTML(message);
}

void handleCalibration1() {
  if (server.hasArg("known")) {
    float knownWeight = server.arg("known").toFloat();
    
    if (!scale1.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 1 non prête. Vérifiez les connexions.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    float raw = scale1.read_average(10);
    
    if (!isValidFloat(raw) || raw == 0.0f) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Valeur invalide de la balance. Réessayez.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    if (!safePreferencesBegin(true)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur d'accès aux préférences.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    float oldOffset = preferences.getFloat("offset1", 0.0);
    safePreferencesEnd();
    
    float newFactor = (raw - oldOffset) / knownWeight;
    
    if (!isValidFactor(newFactor)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Facteur de calibration invalide calculé.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Réessayer</a> ";
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
      Serial.printf("ERROR: Calibration write verify failed! Wrote %.6f, read %.6f\n", 
                    newFactor, readback);
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ La vérification d'écriture a échoué.";
      msg += "</div><br><a href='/calibrate1' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    cfg.factor1 = newFactor;
    scale1.set_scale(newFactor);
    
    String msg = "<div class='header'><h1>✅ Calibration Réussie</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎉 Balance 1 calibrée avec succès !</div>";
    msg += "<div class='info-card'><strong>Facteur de calibration</strong>" + String(newFactor, 6) + "</div>";
    msg += "<br><a href='/weight1' class='btn'>Tester la balance</a> ";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String form = "<div class='header'><h1>Calibration Balance 1</h1></div>";
    form += "<div class='content'>";
    form += "<div class='alert'>📏 Placez un poids connu sur la balance et entrez sa valeur en grammes.</div>";
    form += "<form action='/calibrate1' method='get'>";
    form += "<label><strong>Poids connu (grammes) :</strong></label>";
    form += "<input type='number' step='0.01' name='known' required>";
    form += "<br><br><input type='submit' value='🔧 Calibrer'>";
    form += "</form>";
    form += "<br><a href='/' class='btn'>Retour</a>";
    form += "</div>";
    sendCommonHTML(form);
  }
}

void handleCalibration2() {
  if (server.hasArg("known")) {
    float knownWeight = server.arg("known").toFloat();
    
    if (!scale2.is_ready()) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Balance 2 non prête. Vérifiez les connexions.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    float raw = scale2.read_average(10);
    
    if (!isValidFloat(raw) || raw == 0.0f) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Valeur invalide de la balance. Réessayez.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    if (!safePreferencesBegin(true)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Erreur d'accès aux préférences.";
      msg += "</div><br><a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    float oldOffset = preferences.getFloat("offset2", 0.0);
    safePreferencesEnd();
    
    float newFactor = (raw - oldOffset) / knownWeight;
    
    if (!isValidFactor(newFactor)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Facteur de calibration invalide calculé.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Réessayer</a> ";
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
      Serial.printf("ERROR: Calibration write verify failed! Wrote %.6f, read %.6f\n", 
                    newFactor, readback);
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ La vérification d'écriture a échoué.";
      msg += "</div><br><a href='/calibrate2' class='btn'>Réessayer</a> ";
      msg += "<a href='/' class='btn'>Retour</a></div>";
      sendCommonHTML(msg);
      return;
    }
    
    cfg.factor2 = newFactor;
    scale2.set_scale(newFactor);
    
    String msg = "<div class='header'><h1>✅ Calibration Réussie</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎉 Balance 2 calibrée avec succès !</div>";
    msg += "<div class='info-card'><strong>Facteur de calibration</strong>" + String(newFactor, 6) + "</div>";
    msg += "<br><a href='/weight2' class='btn'>Tester la balance</a> ";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String form = "<div class='header'><h1>Calibration Balance 2</h1></div>";
    form += "<div class='content'>";
    form += "<div class='alert'>📏 Placez un poids connu sur la balance et entrez sa valeur en grammes.</div>";
    form += "<form action='/calibrate2' method='get'>";
    form += "<label><strong>Poids connu (grammes) :</strong></label>";
    form += "<input type='number' step='0.01' name='known' required>";
    form += "<br><br><input type='submit' value='🔧 Calibrer'>";
    form += "</form>";
    form += "<br><a href='/' class='btn'>Retour</a>";
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
    
    float rawZero = scale1.read_average(10);
    
    if (!isValidFloat(rawZero)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Valeur invalide de la balance.";
      msg += "</div><br><a href='/weight1' class='btn'>Réessayer</a> ";
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
    
    preferences.putFloat("offset1", rawZero);
    safePreferencesEnd();
    
    cfg.offset1 = rawZero;
    
    String msg = "<div class='header'><h1>✅ Tare Effectuée</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎯 Balance 1 remise à zéro !</div>";
    msg += "<div class='info-card'><strong>Offset</strong>" + String(rawZero, 2) + "</div>";
    msg += "<br><a href='/weight1' class='btn'>Peser à nouveau</a> ";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String message = "<div class='header'><h1>⚖️ Balance 1</h1></div>";
    message += "<div class='content'>";
    
    if (scale1.is_ready()) {
      float raw = scale1.read_average(10);
      int weight = -1;
      
      if (isValidFloat(raw) && cfg.factor1 != 0.0f) {
        weight = (int)((raw - cfg.offset1) / cfg.factor1);
      }
      
      if (isValidWeight(weight)) {
        message += "<div class='weight-display'>";
        message += "<div class='weight-value'>" + String(weight) + " g</div>";
        message += "<div class='weight-label'>Poids mesuré</div>";
        message += "</div>";
      } else {
        message += "<div class='alert alert-danger'>⚠️ Poids hors limites ou erreur de lecture</div>";
      }
      
      message += "<div class='info-card'><strong>Valeur brute</strong>" + String(raw, 2) + "</div>";
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
    
    float rawZero = scale2.read_average(10);
    
    if (!isValidFloat(rawZero)) {
      String msg = "<div class='header'><h1>Erreur</h1></div>";
      msg += "<div class='content'><div class='alert alert-danger'>";
      msg += "⚠️ Valeur invalide de la balance.";
      msg += "</div><br><a href='/weight2' class='btn'>Réessayer</a> ";
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
    
    preferences.putFloat("offset2", rawZero);
    safePreferencesEnd();
    
    cfg.offset2 = rawZero;
    
    String msg = "<div class='header'><h1>✅ Tare Effectuée</h1></div>";
    msg += "<div class='content'>";
    msg += "<div class='alert'>🎯 Balance 2 remise à zéro !</div>";
    msg += "<div class='info-card'><strong>Offset</strong>" + String(rawZero, 2) + "</div>";
    msg += "<br><a href='/weight2' class='btn'>Peser à nouveau</a> ";
    msg += "<a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String message = "<div class='header'><h1>⚖️ Balance 2</h1></div>";
    message += "<div class='content'>";
    
    if (scale2.is_ready()) {
      float raw = scale2.read_average(10);
      int weight = -1;
      
      if (isValidFloat(raw) && cfg.factor2 != 0.0f) {
        weight = (int)((raw - cfg.offset2) / cfg.factor2);
      }
      
      if (isValidWeight(weight)) {
        message += "<div class='weight-display'>";
        message += "<div class='weight-value'>" + String(weight) + " g</div>";
        message += "<div class='weight-label'>Poids mesuré</div>";
        message += "</div>";
      } else {
        message += "<div class='alert alert-danger'>⚠️ Poids hors limites ou erreur de lecture</div>";
      }
      
      message += "<div class='info-card'><strong>Valeur brute</strong>" + String(raw, 2) + "</div>";
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
    msg += "<div class='alert'>🎉 Nom du device changé avec succès !</div>";
    msg += "<div class='info-card'><strong>Nouveau nom</strong>" + newName + "</div>";
    msg += "<div class='alert'>⚠️ Redémarrez le device pour que le WiFi AP utilise le nouveau nom.</div>";
    msg += "<br><a href='/' class='btn'>Retour au menu</a>";
    msg += "</div>";
    sendCommonHTML(msg);
  } else {
    String form = "<div class='header'><h1>📝 Nom du Device</h1></div>";
    form += "<div class='content'>";
    form += "<div class='alert'>Le nom du device est utilisé pour l'identificateur WiFi et MQTT.</div>";
    form += "<form action='/setDeviceName' method='get'>";
    form += "<label><strong>Nouveau nom (max 32 caractères) :</strong></label>";
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
  content += "<div class='alert'>⚠️ Attention : Ne débranchez pas le device pendant la mise à jour !</div>";
  content += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  content += "<label><strong>Sélectionnez le fichier .bin :</strong></label><br><br>";
  content += "<input type='file' name='update' accept='.bin' required>";
  content += "<br><br><input type='submit' value='⬆️ Mettre à Jour'>";
  content += "</form>";
  content += "<br><a href='/' class='btn'>Retour</a>";
  content += "</div>";
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

  // ✅ ReadOnly suffisant
  if (!safePreferencesBegin(true)) {
    Serial.println("WARNING: Failed to read preferences in AP mode, using defaults");
    cfg.deviceName = "F7";
    cfg.mqtt_server = "ratamuse.hopto.org";
    cfg.mqtt_port = 1885;
  } else {
    cfg.deviceName = preferences.getString("deviceName", "F7");
    cfg.mqtt_server = preferences.getString("mqtt_server", "ratamuse.hopto.org");
    cfg.mqtt_port = preferences.getInt("mqtt_port", 1885);
    safePreferencesEnd();
  }

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

  uint64_t chipid = ESP.getEfuseMac();
  mqtt_client_id = "device-" + String((uint32_t)(chipid & 0xFFFFFFFF), HEX);

  if (!safeModemStart()) {
    Serial.println("Modem failed init");
    emergencySleep();
  }

  loadConfigSafe();

  mqtt_topic_temp1 = cfg.deviceName + "/<>/temp1/<>/data/0";
  mqtt_topic_scaleA = cfg.deviceName + "/<>/scaleA/<>/data/0";
  mqtt_topic_scaleB = cfg.deviceName + "/<>/scaleB/<>/data/0";
  mqtt_topic_battery = cfg.deviceName + "/<>/battery/<>/data/0";
  mqtt_topic_signal = cfg.deviceName + "/<>/signal/<>/data/0";
  mqtt_topic_position = cfg.deviceName + "/<>/position/<>/data/0";
  mqtt_topic_position_approx = cfg.deviceName + "/<>/posap/<>/data/0";
  mqtt_topic_diagnostics = cfg.deviceName + "/<>/diagnostics/<>/data/0";

  scale1.begin(dataPin1, clockPin1);
  scale1.set_scale(cfg.factor1);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  scale2.begin(dataPin2, clockPin2);
  scale2.set_scale(cfg.factor2);
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
    float raw = scale1.read_average(10);
    if (isValidFloat(raw) && cfg.factor1 != 0.0f) {
      scaleA = (int)((raw - cfg.offset1) / cfg.factor1);
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
    float raw = scale2.read_average(10);
    if (isValidFloat(raw) && cfg.factor2 != 0.0f) {
      scaleB = (int)((raw - cfg.offset2) / cfg.factor2);
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
  StaticJsonDocument<256> diagDoc;
  diagDoc["firmware"] = FIRMWARE_VERSION;
  diagDoc["crash_count"] = crashCount;
  diagDoc["last_crash_type"] = lastCrashType;
  diagDoc["nvs_corrupted"] = nvs_corrupted;
  char diagBuffer[256];
  serializeJson(diagDoc, diagBuffer);
  safePublish(mqtt_topic_diagnostics, diagBuffer);

  // ✅ Checkpoint 3 : Après publications
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

  xTaskCreatePinnedToCore(appTask, "appTask", 10000, NULL, 1, &appTaskHandle, 0);

  if (appTaskHandle) esp_task_wdt_add(appTaskHandle);

  if (digitalRead(wifiPin) == LOW) setupAPModeInit();
  else setupNormalInit();
}
