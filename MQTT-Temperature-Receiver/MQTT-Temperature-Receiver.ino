#include <ESP8266WiFi.h>       // https://github.com/esp8266/Arduino
#include <Wire.h>              // https://github.com/esp8266/Arduino (I2C)
#include <WiFiManager.h>       // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>      // https://pubsubclient.knolleary.net/
#include <hd44780.h>           // https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

// ==============================================
//  Configurable values
// ==============================================

// MQTT broker's host name + port number
#define MQTT_BROKER_HOST_NAME "mqtt.example.com"
#define MQTT_BROKER_PORT 1883

// Topic name where from where to receive the temperature
#define MQTT_TOPIC "mqtt-iot-workshop/device-0/temperature"

// Access point name if no WiFi credentials have been stored
#define AP_NAME "AP-receiver"

// Alarm Threshold: Turn on LED if temperature is higher
#define TEMP_WARNING_THRESHOLD 30.0

// LCD geometry
#define LCD_COLS 16
#define LCD_ROWS 2

// ==============================================
//  Global variables
// ==============================================

WiFiClient wiFiClient;
PubSubClient mqttClient(wiFiClient);
hd44780_I2Cexp lcd;

// ==============================================

void setup() {
  Serial.begin(115200);

  int status = lcd.begin(LCD_COLS, LCD_ROWS);
  if (status) {
    // non zero status means it was unsuccesful
    status = -status; // convert negative status value to positive number
    hd44780::fatalError(status); // does not return
  }
  connectWifi();
  mqttClient.setServer(MQTT_BROKER_HOST_NAME, MQTT_BROKER_PORT);
  mqttClient.setCallback(mqttCallback);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  connectMqtt();
  mqttClient.loop();
  // Repeat every 10 seconds
  delay(10000);
}

/*
  MQTT callback function that will receive the temperature on the given MQTT topic
  and will show that value on the LCD
*/
void mqttCallback(const char topic[], byte* payload, unsigned int length) {
  // Caveat: We don't check the topic here!
  String temperature = asString(payload, length);
  Serial.println("Incoming message: " + temperature);
  lcd.clear();
  lcd.print("Temperature:");
  lcd.setCursor(0, 1); // Move to 2nd line
  lcd.print(temperature + " ");
  lcd.print((char) 223); // degree sign
  lcd.print("C");

  // Turn builtin LED on (= LOW!) / off (= HIGH!) if temp is above / below threshold
  digitalWrite(LED_BUILTIN, temperature.toFloat() > TEMP_WARNING_THRESHOLD ? LOW : HIGH);
}

/**
 Parses the MQTT payload and returns it as trimmed String
*/
String asString(byte* payload, unsigned int length) {
  String result;
  for (int i = 0; i < length; i++) {
    char c = (char) payload[i];
    if (c == ' ') {
      // skip white space
      continue;
    }
    result += c;
  }
  return result;
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
  wifiManager.autoConnect(AP_NAME);

  // Now you are connected to the WiFi
  Serial.print("WiFi connected with local IP ");
  Serial.println(WiFi.localIP());
}

/*
  (Re)connects to a MQTT broker and subscribe to the topic MQTT_TOPIC
*/
void connectMqtt() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    if (mqttClient.connect(generateClientID())) {
      mqttClient.subscribe(MQTT_TOPIC);
      Serial.println("MQTT connected + subscribed");
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
