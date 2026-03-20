#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Change the SSID and PASSWORD
const int AP_COUNT = 3;
const char *AP_SSID_LIST[] = {"01", "02", "03"};
const char *AP_PASSWD = "taist_aiot";
const int NUM_DATA = 10;

int AP_scan(bool *ssid_list);
bool AP_connect(int ap_idx, float *ftm_list, int *rssi_list);

bool ssid_list[AP_COUNT] = {false};

// FTM measurement
const uint8_t FTM_FRAME_COUNT = 16;
const uint16_t FTM_BURST_PERIOD = 2;

void onFtmReport(arduino_event_t *event);

SemaphoreHandle_t ftmSemaphore; // Semaphore to signal when FTM Report has been received
bool ftmSuccess = true; // Status of the received FTM Report
volatile float tmp_ftm_value = 0.0;

// Internet connectivity
const char *WIFI_SSID = "YOUR_HOTSPOT";          // SSID of AP that has FTM Enabled
const char *WIFI_PASSWD = "YOUR_PASSWD"; // STA Password

const char *MQTT_BROKER = "broker.emqx.io";
const char *PUB_TOPIC = "taist/aiot/YOUR_TEAM/data";
const char *SUB_TOPIC = "taist/aiot/YOUR_TEAM/cmd";

void mqtt_callback(char* topic, byte* payload, unsigned int length);
bool upload_data(int ap_idx, float *ftm_list, int *rssi_list);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// User interface
#define LED_PIN    18
Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

typedef enum {
  EV_NONE,
  EV_BUTTON
} event_t;

typedef enum {
  ST_BOOT,
  ST_READY,
  ST_REPORT
} state_t;

event_t event_check(void);

state_t cur_state = ST_BOOT;

void setup() {
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  pinMode(2, OUTPUT);
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(100, 100, 0)); // YELLOW in ST_BOOT
  pixel.show();

  // scan network at boot
  int ap_count = AP_scan(ssid_list);
  if (ap_count > 0) {
    cur_state = ST_READY;
    Serial.println("ST_BOOT -> ST_READY");
    pixel.setPixelColor(0, pixel.Color(0, 100, 0)); // GREEN in ST_READY
    pixel.show();
  }
  ftmSemaphore = xSemaphoreCreateBinary();     // Create binary semaphore
  WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);
}

void loop() {
  static float ftm_list[NUM_DATA] = {0.0};
  static int rssi_list[NUM_DATA] = {-100};
  static int ap_idx = 0;
  static int ready_idx = 0;

  event_t ev = event_check();
  switch(cur_state) {
    case ST_BOOT:
      if (ev == EV_BUTTON) {
        int ap_count = AP_scan(ssid_list);
        if (ap_count > 0) {
          cur_state = ST_READY;
          Serial.println("ST_BOOT -> ST_READY");
          pixel.setPixelColor(0, pixel.Color(0, 100, 0)); // GREEN in ST_READY
          pixel.show();
        }
      }
      break;
    case ST_READY:
      if (ev == EV_BUTTON) {
        while (!ssid_list[ap_idx]) {
          ap_idx = (ap_idx + 1) % AP_COUNT;
        } 
        pixel.setPixelColor(0, pixel.Color(0, 100, 100)); // LIGHT BLUE in connect
        pixel.show();
        if (AP_connect(ap_idx, ftm_list, rssi_list)) {
          ready_idx = ap_idx;
          cur_state = ST_REPORT;
          pixel.setPixelColor(0, pixel.Color(0, 0, 100)); // BLUE in connect
          pixel.show();
        } else {
          pixel.setPixelColor(0, pixel.Color(0, 100, 0)); // GREEN in connect
          pixel.show();
        }
        ap_idx = (ap_idx + 1) % AP_COUNT;
      }
      break;
    case ST_REPORT:
      if (ev == EV_BUTTON) {
        pixel.setPixelColor(0, pixel.Color(100, 0, 0)); // RED in connect
        pixel.show();
        if (upload_data(ready_idx, ftm_list, rssi_list)) {
          cur_state = ST_READY;
          pixel.setPixelColor(0, pixel.Color(0, 100, 0)); // GREEN in connect
          pixel.show();
        }
      }
      break;
    default:
      cur_state = ST_BOOT;
      pixel.setPixelColor(0, pixel.Color(100, 100, 0)); // YELLOW in ST_BOOT
      pixel.show();
  }
  ev = EV_NONE;
  delay(100);
}

// scan for matched AP
int AP_scan(bool *ssid_list) {
  int ap_count = 0;
  Serial.println("Start scanning");
  for (int i=0; i < AP_COUNT; i++) {
    ssid_list[i] = false; 
  }
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No AP found!");
    for (int i=0; i < AP_COUNT; i++) {
      ssid_list[i] = false;
    }
  }
  for (int i=0; i < n; i++) {
    for (int j=0; j < AP_COUNT; j++) {
      if (ssid_list[j]) {
        continue;
      }
      if (strcmp(AP_SSID_LIST[j], WiFi.SSID(i).c_str()) == 0) {
        ssid_list[j] = true;
        ap_count++;
        Serial.printf("%s: %d matched\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i) );
        break;
      }
    }
  }
  return ap_count;
}

// connect to AP
bool AP_connect(int ap_idx, float *ftm_list, int *rssi_list) {
  WiFi.begin(AP_SSID_LIST[ap_idx], AP_PASSWD);
  while(WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, !digitalRead(2));
    delay(100);
  }
  Serial.printf("%s: ", AP_SSID_LIST[ap_idx]);
  for (int j=0; j < NUM_DATA; j++) {
    if (!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD)) {
      Serial.println("FTM Error: Initiate Session Failed");
      return false;
    }
    if (xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && ftmSuccess) {
      ftm_list[j] = tmp_ftm_value;
      tmp_ftm_value = 0.0;
      rssi_list[j] = WiFi.RSSI();
      Serial.printf("%f/%d, ", ftm_list[j], rssi_list[j]);
    } else {
      ftm_list[j] = 0.0;
      rssi_list[j] = WiFi.RSSI();
    }
    delay(100);
  }
  Serial.println();
  WiFi.disconnect();
  return true;
}

// FTM measurement
void onFtmReport(arduino_event_t *event) {
  const char *status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
  wifi_event_ftm_report_t *report = &event->event_info.wifi_ftm_report;
  if (report == NULL) {
    Serial.println("Error FTM");
    ftmSuccess = false;
    xSemaphoreGive(ftmSemaphore);   // Signal that report is received
  }
  ftmSuccess = report->status == FTM_STATUS_SUCCESS;
  if (ftmSuccess) {
    tmp_ftm_value = (float)report->dist_est / 100.0;
  } else {
    tmp_ftm_value = 0.0;
    Serial.println("FTM Error: ");
  }
  free(report->ftm_report_data);
  xSemaphoreGive(ftmSemaphore);   // Signal that report is received
}

// connect and report to MQTT
bool upload_data(int ap_idx, float *ftm_list, int *rssi_list) {
  Serial.println("Start uploading");
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, !digitalRead(2));
    delay(100);
  }
  Serial.println("WiFi connected");
  // MQTT
  mqttClient.setServer(MQTT_BROKER, 1883);
  mqttClient.setCallback(mqtt_callback);
  char dev_id[64];
  sprintf(dev_id, "DEV%d", random());
  if (!mqttClient.connect(dev_id)) {
    return false;
  }
  //mqttClient.subscribe(SUB_TOPIC);
  Serial.println("MQTT connected");
  // JSON
  JsonDocument json_doc;
  char json_txt[256];
  json_doc["AP"] = AP_SSID_LIST[ap_idx];
  for (int i=0; i < NUM_DATA; i++) {
    json_doc["FTM"][i] = ftm_list[i]; 
    json_doc["RSSI"][i] = rssi_list[i];
  }
  json_doc["tstamp"] = millis();
  serializeJson(json_doc, json_txt);
  mqttClient.publish(PUB_TOPIC, json_txt);
  mqttClient.loop();
  mqttClient.disconnect();
  Serial.println(json_txt);
  delay(100);
  WiFi.disconnect();
  return true;
}

// on MQTT message
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char buf[256];
  memcpy(buf, payload, length);
  buf[length] = 0;
  Serial.printf("%s: %s\n", topic, buf);
}

// check event
event_t event_check(void) {
  event_t new_event = EV_NONE;
  static bool btn_pressed = false;

  if (btn_pressed) {
    if (digitalRead(0) != 0) { 
      new_event = EV_BUTTON; // on released
      btn_pressed = false;
      Serial.println("Button released");
    }
  } else {
    if (digitalRead(0) == 0) {
      btn_pressed = true;
      Serial.println("Button pressed");
    }
  }
  return new_event;
}