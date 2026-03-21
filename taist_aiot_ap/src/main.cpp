#include <Arduino.h>
#include "WiFi.h"

// Change the SSID and PASSWORD here if needed
const char *WIFI_FTM_SSID = "YOUR_AP";
const char *WIFI_FTM_PASS = "YOUR_PASSWD";
const int WIFI_FTM_CHANNEL = 9;

void setup() {
  Serial.begin(115200);

  Serial.println("Starting SoftAP with FTM Responder support");
  // Enable AP with FTM support (last argument is 'true')
  WiFi.softAP(WIFI_FTM_SSID, WIFI_FTM_PASS, WIFI_FTM_CHANNEL, 0, 4, true); // use channel 8 or 9 that should be not noisy
}

void loop() {
  delay(1000);
}