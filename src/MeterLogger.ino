/*
   Program for isolating and publishing smartmeter sml messages to a mqtt broker by using a esp32.
*/

#define LED_PIN 2
#define OUTFEED_RX_PIN 14 // for NodeMCU: GPIO4 = D1 
#define OUTFEED_TX_PIN 26 // for NodeMCU: GPIO5 = D2
#define SOLAR_RX_PIN 12 // for NodeMCU: GPIO4 = D1 
#define SOLAR_TX_PIN 27 // for NodeMCU: GPIO5 = D2
#define DEBUGLEVEL NONE

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10       /* Time ESP32 will go to sleep (in seconds) */

#include <WiFi.h>
#include <ArduinoOTA.h>

#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
#include <HardwareSerial.h>
#include <SMLParser.h>
#include <DebugUtils.h>
#include <ArduinoJson.h>

//#include "Config.h" // make your own config file or remove this line and use the following lines
const char* clientId = "Energy";
const char* mqtt_server = "192.168.1.21";
#include "WifiCredentials.h" // const char* ssid = "MySSID"; const char* WifiPassword = "MyPw";
#include "OTACredentials.h" // const char* OtaPassword = "MyPw";
IPAddress ip(192, 168, 1, 7); // Static IP
IPAddress dns(192, 168, 1, 1); // most likely your router
IPAddress gateway(192, 168, 1, 1); // most likely your router
IPAddress subnet(255, 255, 255, 0);

unsigned long lastUpdated;
unsigned long lastLed;
uint8_t stateLed = HIGH;
boolean updateActive;
RTC_DATA_ATTR unsigned long bootCount;
RTC_DATA_ATTR boolean enableUpdate;

SMLParser OutfeedMeter(1);
SMLParser SolarMeter(2);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void update_led(unsigned long intervallHigh, unsigned long intervallLow) {
  if (stateLed == HIGH && millis() - lastLed >= intervallHigh) {
    stateLed = LOW;
    lastLed = millis();
  }
  else if (stateLed == LOW && millis() - lastLed >= intervallLow) {
    stateLed = HIGH;
    lastLed = millis();
  }
  digitalWrite(LED_PIN, stateLed);
}

void setup_wifi() {
  delay(10);
  DEBUGPRINTNONE("Connecting to ");
  DEBUGPRINTLNNONE(ssid);
  WiFi.config(ip, dns, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, WifiPassword);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    DEBUGPRINTNONE(".");
    counter ++;
    unsigned long timer = millis();
    while (millis() - timer <= 500) {
      update_led(100, 100);
    }
    //delay(500);
    if (counter >= 20) {
      counter = 0;
      DEBUGPRINTLNNONE("Retry");
      WiFi.disconnect();
      while (WiFi.status() == WL_CONNECTED) {
        DEBUGPRINTNONE(".");
        update_led(100, 100);
        delay(10);
      }
      WiFi.begin(ssid, WifiPassword);
    }
  }
  DEBUGPRINTLNNONE("WiFi connected");
  DEBUGPRINTNONE("IP address: ");
  DEBUGPRINTLNNONE(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char* topic1 = "/enableUpdate";
  char* path = (char *) malloc(1 + strlen(clientId) + strlen(topic1) );
  strcpy(path, clientId);
  strcat(path, topic1);
  //if (topic == path) {
  DEBUGPRINTLNNONE("NewMessage");
  DEBUGPRINTLNNONE(topic);
  DEBUGPRINTLNNONE(length);
  free(path);
  enableUpdate = true;
  topic = "/enableUpdateAck";
  path = (char *) malloc(1 + strlen(clientId) + strlen(topic) );
  strcpy(path, clientId);
  strcat(path, topic);
  mqttClient.publish(path, "true");
  free(path);
  delay(100);
  //}
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiReconnect();
    }
    DEBUGPRINTNONE("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(clientId)) {
      DEBUGPRINTLNNONE("connected");
      // ... and resubscribe
      char* topic = "/enableUpdate";
      char* path = (char *) malloc(1 + strlen(clientId) + strlen(topic) );
      strcpy(path, clientId);
      strcat(path, topic);
      DEBUGPRINTLNNONE(path);
      mqttClient.subscribe(path);
      free(path);
    } else {
      DEBUGPRINTNONE("failed, rc=");
      DEBUGPRINTNONE(mqttClient.state());
      DEBUGPRINTLNNONE(" try again in 1 seconds");
      // Wait 5 seconds before retrying
      unsigned long timer = millis();
      while (millis() - timer <= 1000) {
        update_led(250, 250);
      }
      //delay(1000);
    }
  }
}

void wifiReconnect() {
  WiFi.disconnect();
  while (WiFi.status() == WL_CONNECTED) {
    DEBUGPRINTNONE(".");
    delay(10);
  }
  DEBUGPRINTNONE("Reconnecting to ");
  DEBUGPRINTLNNONE(ssid);
  int counter = 0;
  WiFi.begin(ssid, WifiPassword);
  while (WiFi.status() != WL_CONNECTED and counter < 20) {
    DEBUGPRINTNONE(".");
    counter ++;
    unsigned long timer = millis();
    while (millis() - timer <= 500) {
      update_led(100, 100);
    }
    //delay(500);
  }
  DEBUGPRINTLNNONE("WiFi connected");
  DEBUGPRINTNONE("IP address: ");
  DEBUGPRINTLNNONE(WiFi.localIP());
}

void setup() {
  bootCount ++;
  updateActive = enableUpdate;
  lastUpdated = millis();
  lastLed = millis();
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  DEBUGPRINTLNNONE("\nHardware serial started");
  DEBUGPRINTLNNONE(bootCount);
  OutfeedMeter.Begin(9600, SERIAL_8N1, OUTFEED_RX_PIN, OUTFEED_TX_PIN, true);
  //pinMode(OUTFEED_RX_PIN, INPUT_PULLDOWN);
  DEBUGPRINTLNNONE("\nOutfeedMeter serial started");
  SolarMeter.Begin(9600, SERIAL_8N1, SOLAR_RX_PIN, SOLAR_TX_PIN, true);
  //pinMode(SOLAR_RX_PIN, INPUT_PULLDOWN);
  DEBUGPRINTLNNONE("\nSolarMeter serial started");
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
  // --------------------------------------------------------------------- OTA

  // Port defaults to 8266
  if (enableUpdate) {
    ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(clientId);

    // No authentication by default
    ArduinoOTA.setPassword(OtaPassword);

    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      enableUpdate = false;
      updateActive = false;
      DEBUGPRINTDEBUG("WiFi.disconnect");
      WiFi.disconnect();
      while (WiFi.status() == WL_CONNECTED) {
        DEBUGPRINTDEBUG('.');
      }
      DEBUGPRINTLNDEBUG("");
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      Serial.println("");
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
  }
}

void loop() {
  char Data[256];
  ArduinoOTA.handle();
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();

  OutfeedMeter.loop();
  SolarMeter.loop();
  if (millis() - lastUpdated >= 10000 && OutfeedMeter.getAvailable() && SolarMeter.getAvailable()) {
    OutfeedMeter.resetAvailable();
    SolarMeter.resetAvailable();
    double value = SolarMeter.getTotalSupply() - OutfeedMeter.getTotalSupply();
    DEBUGPRINTNONE("Eigenverbrauch: ");
    DEBUGPRINTNONE(value);
    DEBUGPRINTNONE("kWh ");
    value = (value / SolarMeter.getTotalSupply()) * 100;
    DEBUGPRINTNONE(value);
    DEBUGPRINTLNNONE("%");

    value = SolarMeter.getActualPower();
    DEBUGPRINTNONE("Erzeugung: ");
    DEBUGPRINTNONE(value, 0);
    DEBUGPRINTLNNONE("W");

    value = OutfeedMeter.getActualPower();
    DEBUGPRINTNONE("Bezug: ");
    DEBUGPRINTNONE(value, 0);
    DEBUGPRINTLNNONE("W");

    value = OutfeedMeter.getActualPower() - SolarMeter.getActualPower();
    DEBUGPRINTNONE("Hausverbrauch: ");
    DEBUGPRINTNONE(value, 0);
    DEBUGPRINTLNNONE("W");

    const size_t capacity = JSON_OBJECT_SIZE(2) + 4*JSON_OBJECT_SIZE(3);
    DynamicJsonDocument doc(capacity);

    JsonObject Outfeed = doc.createNestedObject("Outfeed");
    sprintf(Data, "%ld", OutfeedMeter.getActualPower());
    Outfeed["actualPower"] = Data;
    sprintf(Data, "%lf", OutfeedMeter.getTotalConsumption());
    Outfeed["totalConsumption"] = Data;
    sprintf(Data, "%lf", OutfeedMeter.getTotalSupply());
    Outfeed["totalSupply"] = Data;
    
    JsonObject Solar = doc.createNestedObject("Solar");
    sprintf(Data, "%ld", SolarMeter.getActualPower());
    Solar["actualPower"] = Data;
    sprintf(Data, "%lf", SolarMeter.getTotalConsumption());
    Solar["totalConsumption"] = Data;
    sprintf(Data, "%lf", SolarMeter.getTotalSupply());
    Solar["totalSupply"] = Data;

    serializeJson(doc, Data, sizeof(Data));
    char* topic = "/MeterData";
    char* path = (char *)malloc(1 + strlen(clientId) + strlen(topic));
    strcpy(path, clientId);
    strcat(path, topic);
    if (!mqttClient.publish(path, Data, true)){
      DEBUGPRINTLNNONE("MQTT publish failed");
    }
    free(path);

    DEBUGPRINTDEBUG(topic);
    DEBUGPRINTDEBUG(" ");
    DEBUGPRINTLNDEBUG(Data);

    delay(100);
    update_led(500, 500);

    lastUpdated = millis();
  }
  else if (millis() - lastUpdated < 500) {
    update_led(500, 500);
  }
  else{
    update_led(100, 1000);
  }

  if ((!updateActive && enableUpdate) || (millis() - lastUpdated >= 60000))  {

    DEBUGPRINTLNDEBUG("MQTT disconnect");
    mqttClient.disconnect();
    while (mqttClient.connected()) {
      DEBUGPRINTDEBUG(".");
    }
    DEBUGPRINTLNDEBUG("");

    DEBUGPRINTDEBUG("WiFi.disconnect");
    WiFi.disconnect();
    while (WiFi.status() == WL_CONNECTED) {
      DEBUGPRINTDEBUG(".");
    }
    DEBUGPRINTLNDEBUG("");

    DEBUGPRINTLNDEBUG("Sleep");

    esp_sleep_enable_timer_wakeup(1 * uS_TO_S_FACTOR);
    delay(1 * 1000);
    esp_deep_sleep_start();
  }
}
