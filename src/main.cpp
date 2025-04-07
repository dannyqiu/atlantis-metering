#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> // For HTTPS
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffsetSec = -8 * 60 * 60; // GMT offset in seconds (e.g., -18000 for EST)
const long  daylightOffsetSec = 0; // Daylight saving time offset in seconds (e.g., 3600 for 1 hour)

const char* ssid = "bae area control";
const char* password = "baebaebae";
const char* localhostUrl = "http://192.168.1.123:8080";
const char* serverUrl = localhostUrl;

constexpr int kSerialBaudRate = 9600;
constexpr int kWifiConnectionTimeoutMs = 60000;

constexpr int kPulseCounterPin = 22;
volatile unsigned long pulseCount = 0;

void IRAM_ATTR pulseCounterISR() {
  ++pulseCount;
}

// Forward declare functions
bool connectToWifi();
bool sendJsonData(String, String);
void printLocalTime();
unsigned long getLocalTimeEpoch();

void setup() {
  Serial.begin(kSerialBaudRate);
  pinMode(kPulseCounterPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(kPulseCounterPin), pulseCounterISR, RISING); // Change RISING to FALLING or CHANGE as needed

  // Delay so that serial connection can start.
  delay(2500);

  bool wifi_connected = connectToWifi();
  if (!wifi_connected) {
    ESP.restart();
  }

  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
  printLocalTime();
}

void loop() {
  static unsigned long lastPrintTime = 0;
  static unsigned long lastSendTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastPrintTime >= 5000) { // Print every 5s
    Serial.print("Pulse Count: ");
    Serial.println(pulseCount);
    lastPrintTime = currentTime;
  }
  if (currentTime - lastSendTime >= 10000) { // Send network request every 10s
    sendJsonData(localhostUrl, "{\"pulseCount\":" + String(pulseCount) + "}"); //send another example post request.
    lastSendTime = currentTime;
  }
}

bool connectToWifi() {
  Serial.println();
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < kWifiConnectionTimeoutMs) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("");
    Serial.println("WiFi connection timed out.");
    return false;
  }
}

void printHeaders(HTTPClient& http) {
  Serial.println("Headers:");
  for (int i = 0; i < http.headers(); ++i) {
    Serial.print(http.headerName(i));
    Serial.print(" : ");
    Serial.println(http.header(i));
  }
}

void printResponse(HTTPClient& http) {
  Serial.println("Response:");
  Serial.println(http.getString());
}

void printSslError(WiFiClientSecure& httpsClient) {
  char buffer[1024];
  httpsClient.lastError(buffer, 1024);
  Serial.print("SSL Error: ");
  Serial.println(buffer);
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "Current Time: %A, %B %d %Y %H:%M:%S %Z");
}

unsigned long getLocalTimeEpoch() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0; // Return 0 on failure
  }
  return mktime(&timeinfo); // Convert tm struct to epoch time
}

bool sendJsonData(String url, String json_data) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi Disconnected");
    return false;
  }

  WiFiClientSecure httpsClient; // Use WiFiClientSecure for HTTPS
  HTTPClient http;
  bool is_https = url.startsWith("https:");
  if (is_https) {
    httpsClient.setInsecure();
    http.begin(httpsClient, url);
  }
  else {
    http.begin(url);
  }
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Follow redirects
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"timestamp\":" + String(getLocalTimeEpoch()) + ", \"data\":" + json_data + "}";

  int httpResponseCode = http.POST(payload);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  printHeaders(http);
  printResponse(http);
  if (httpResponseCode < 0) {
    if (is_https) {
      printSslError(httpsClient);
    }
  }

  http.end();
  return true;
}
