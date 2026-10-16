/*
  ESP32 - Firebase custom LED patterns (LAG නැති version)
  Firebase check කරන එක වෙනම task එකක (core 0) දුවනවා,
  LED රටාව main loop එකේ (core 1) නොනවත්වා දුවනවා.

  Library: ArduinoJson (7.x)

  Firebase: /led -> pattern (0-6), speed (ms), custom ("100000:200,010000:200,...")
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>

const char* WIFI_SSID = "MAHESH";
const char* WIFI_PASS = "200307055";
const char* DB_URL    = "YOUR FIREBASE DETABASE LINK";
const char* DB_AUTH   = "";

const unsigned long POLL_INTERVAL = 2000;

const uint8_t PINS[] = {13, 14, 16, 17, 23, 25};
const uint8_t PIN_COUNT = sizeof(PINS) / sizeof(PINS[0]);

// ---------- Custom pattern ----------
const uint16_t MAX_STEPS = 300;
const size_t MAX_CUSTOM_LEN = 3900;
struct Step { uint8_t mask; uint16_t ms; };
Step steps[MAX_STEPS];
uint16_t stepCount = 0;

// ---------- State (main loop එකේ පමණක් පාවිච්චි වෙනවා) ----------
Preferences prefs;
int pattern = 6;
unsigned long speedMs = 300;
String customStr = "100000:200,010000:200,001000:200,000100:200,000010:200,000001:200";
unsigned long lastStep = 0;
int stepIdx = 0, dir = 1;

// ---------- Task එකෙන් main loop එකට යවන අලුත් අගයන් ----------
SemaphoreHandle_t mtx;
volatile bool hasUpdate = false;
int pPattern = 6;
unsigned long pSpeed = 300;
String pCustom;

bool parseCustom(const String& s) {
  uint16_t n = 0;
  int start = 0;
  while (start < (int)s.length() && n < MAX_STEPS) {
    int comma = s.indexOf(',', start);
    if (comma < 0) comma = s.length();
    String part = s.substring(start, comma);
    part.trim();
    int colon = part.indexOf(':');
    if (colon > 0) {
      String m = part.substring(0, colon);
      long t = part.substring(colon + 1).toInt();
      uint8_t mask = 0;
      for (uint8_t i = 0; i < PIN_COUNT && i < m.length(); i++)
        if (m[i] == '1') mask |= (1 << i);
      steps[n].mask = mask;
      steps[n].ms = constrain(t, 10L, 60000L);
      n++;
    }
    start = comma + 1;
  }
  if (n == 0) return false;
  stepCount = n;
  return true;
}

void allOff() { for (uint8_t i = 0; i < PIN_COUNT; i++) digitalWrite(PINS[i], LOW); }

void saveToFlash() {
  prefs.begin("led", false);
  prefs.putInt("pattern", pattern);
  prefs.putULong("speed", speedMs);
  prefs.putString("custom", customStr);
  prefs.end();
  Serial.println("Flash saved");
}

void loadFromFlash() {
  prefs.begin("led", true);
  pattern   = prefs.getInt("pattern", 6);
  speedMs   = prefs.getULong("speed", 300);
  customStr = prefs.getString("custom", customStr);
  prefs.end();
  parseCustom(customStr);
}

// ---------- Patterns ----------
void runCustom() {
  if (stepCount == 0) return;
  if (millis() - lastStep < steps[stepIdx].ms) return;
  lastStep = millis();
  stepIdx = (stepIdx + 1) % stepCount;
  uint8_t m = steps[stepIdx].mask;
  for (uint8_t i = 0; i < PIN_COUNT; i++) digitalWrite(PINS[i], (m >> i) & 1);
}

void runBuiltin() {
  if (millis() - lastStep < speedMs) return;
  lastStep = millis();
  switch (pattern) {
    case 0: allOff(); break;
    case 1:
      stepIdx = !stepIdx;
      for (uint8_t i = 0; i < PIN_COUNT; i++) digitalWrite(PINS[i], stepIdx);
      break;
    case 2:
      allOff(); digitalWrite(PINS[stepIdx % PIN_COUNT], HIGH);
      stepIdx = (stepIdx + 1) % PIN_COUNT;
      break;
    case 3:
      allOff(); digitalWrite(PINS[stepIdx], HIGH);
      stepIdx += dir;
      if (stepIdx >= PIN_COUNT - 1) { stepIdx = PIN_COUNT - 1; dir = -1; }
      if (stepIdx <= 0)             { stepIdx = 0;             dir = 1;  }
      break;
    case 4:
      stepIdx = !stepIdx;
      for (uint8_t i = 0; i < PIN_COUNT; i++) digitalWrite(PINS[i], (i % 2) == stepIdx);
      break;
    case 5:
      for (uint8_t i = 0; i < PIN_COUNT; i++) digitalWrite(PINS[i], random(2));
      break;
  }
}

// Task එකෙන් ආපු අලුත් අගයන් main loop එකේදී ආරක්ෂිතව apply කරනවා
void applyUpdate() {
  if (!hasUpdate) return;

  xSemaphoreTake(mtx, portMAX_DELAY);
  int p = pPattern;
  unsigned long s = pSpeed;
  String c = pCustom;
  hasUpdate = false;
  xSemaphoreGive(mtx);

  if (c != customStr && (c.length() > MAX_CUSTOM_LEN || !parseCustom(c))) {
    Serial.println("Custom format වැරදියි / දිග වැඩියි - ignore කළා");
    c = customStr;
  }
  pattern = p;
  speedMs = s;
  customStr = c;
  stepIdx = 0; dir = 1;
  saveToFlash();
}

// ---------- Firebase task (core 0) ----------
void fetchTask(void*) {
  for (;;) {
    vTaskDelay(POLL_INTERVAL / portTICK_PERIOD_MS);

    if (WiFi.status() != WL_CONNECTED) { WiFi.reconnect(); continue; }

    String url = String(DB_URL) + "/led.json";
    if (strlen(DB_AUTH) > 0) url += "?auth=" + String(DB_AUTH);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(client, url);

    if (http.GET() == 200) {
      JsonDocument doc;
      if (!deserializeJson(doc, http.getString()) && !doc.isNull()) {
        xSemaphoreTake(mtx, portMAX_DELAY);
        int newPattern = constrain((int)(doc["pattern"] | pattern), 0, 6);
        unsigned long newSpeed = constrain((unsigned long)(doc["speed"] | speedMs), 20UL, 5000UL);
        String newCustom = doc["custom"] | customStr.c_str();

        if (newPattern != pattern || newSpeed != speedMs || newCustom != customStr) {
          pPattern = newPattern;
          pSpeed = newSpeed;
          pCustom = newCustom;
          hasUpdate = true;
        }
        xSemaphoreGive(mtx);
      }
    }
    http.end();
  }
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < PIN_COUNT; i++) pinMode(PINS[i], OUTPUT);
  loadFromFlash();

  mtx = xSemaphoreCreateMutex();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Firebase task එක core 0 එකේ (LED loop එක core 1 එකේ)
  xTaskCreatePinnedToCore(fetchTask, "fb", 12288, NULL, 1, NULL, 0);
}

void loop() {
  if (pattern == 6) runCustom(); else runBuiltin();
  applyUpdate();
  delay(1);
}
