#include <ESP8266WiFi.h>       // https://github.com/esp8266/Arduino
#include <WiFiManager.h>       // https://github.com/tzapu/WiFiManager
#include <OneWire.h>           // https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <PubSubClient.h>      // https://pubsubclient.knolleary.net/

// ==============================================
//  Configurable values
// ==============================================

// PIN on which the One Wire data wire is connected
#define ONE_WIRE_PIN D4

// MQTT broker's host name + port number
#define MQTT_BROKER_HOST_NAME "mqtt.example.com"
#define MQTT_BROKER_PORT 1883

// Topic name where to publish the temperature
#define MQTT_TOPIC "mqtt-iot-workshop/device-0/temperature"

// ==============================================
//  Global variables
// ==============================================

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
WiFiClient wiFiClient;
PubSubClient mqttClient(wiFiClient);

// ==============================================

void setup() {
  Serial.begin(115200);
  connectWifi();
  sensors.begin();
  mqttClient.setServer(MQTT_BROKER_HOST_NAME, MQTT_BROKER_PORT);
}

void loop() {
  sensors.requestTemperatures();
  // Use the 1st sensor (index 0) because we connected only one
  float tempAsFloat = sensors.getTempCByIndex(0);
  char buff[5];
  // http://www.atmel.com/webdoc/avrlibcreferencemanual/group__avr__stdlib_1ga060c998e77fb5fc0d3168b3ce8771d42.html
  char* tempAsString = dtostrf(tempAsFloat, -2, 1, buff);

  Serial.print("Current temperature is ");
  Serial.print(tempAsString);
  Serial.println(" °C");

  Serial.println("Publishing value via MQTT");
  connectMqtt();
  boolean success = mqttClient.publish(MQTT_TOPIC, tempAsString, true);
  if (!success) {
    Serial.println("Publishing failed");
  }
  // Repeat every 10 seconds
  delay(10000);
}

/*
 Establishes a connection to a WiFi.

 This methods uses the WiFi Manager, i.e.
  - If started the first time it opens an ad-hoc Access Point
    where you can enter your Wifi Settings (SSID + password)
  - Otherwhise it connects to a WiFi using the given credentials
*/
void connectWifi() {
  Serial.println("Starting WiFi");
  WiFiManager wifiManager;

  //reset saved settings
  //wifiManager.resetSettings();

  // Connect to previously stored WiFi or open an ad-hoc Access Point
  wifiManager.autoConnect();

  // Now you are connected to the WiFi
  Serial.print("WiFi connected with local IP ");
  Serial.println(WiFi.localIP());
}

/*
 (Re)connects to a MQTT broker
*/
void connectMqtt() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    if (mqttClient.connect(generateClientID())) {
      Serial.println("MQTT connected");
    } else {
      Serial.print("MQTT failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Try again in 5 seconds");
      delay(5000);
    }
  }
}

/**
   Generate a MQTT client ID based on the MAC address
*/
char* generateClientID() {
  String clientName = "esp8266-" + getMacAddress();
  return (char*) clientName.c_str();
}

/*
 Returns the MAC address of the ESP-8266 as a String
 See https://gist.github.com/igrr/7f7e7973366fc01d6393
*/
String getMacAddress() {
  String result;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
