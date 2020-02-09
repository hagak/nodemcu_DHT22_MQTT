#include <Arduino.h>
/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <DHT.h>
/************************* WiFi Access Point *********************************/
#include "secrets.h"

/************************* Adafruit.io Setup *********************************/



/************************* DHT22 SETUP *********************************/
#define DHTPIN 4     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);
float sleepTime = 60e6;
/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, MQTT_CLIENT_ID, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/temperature");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/humidity");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe sampleTime = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/sampleTime", MQTT_QOS_1);

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  // Wait for serial to initialize.
  while (!Serial) { }
  
  Serial.println("Device Started");
  // Initialize device.
  dht.begin();
  // Get temperature event and print its value.
  float temperatureReading = dht.readTemperature(true);
  if (isnan(temperatureReading)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(temperatureReading);
    Serial.println(F("°F"));
  }
  // Get humidity event and print its value.
  float humidityReading = dht.readHumidity();
  if (isnan(humidityReading)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(humidityReading);
    Serial.println(F("%"));
  }
  sampleTime.setCallback(sampleTimeCallback);
  
  mqtt.subscribe(&sampleTime);
  connect();
  mqtt.processPackets(1000);
  
  reportData(temperatureReading,humidityReading);

  //Wait for next reading  
  ESP.deepSleep(sleepTime);  //60 seconds
}

void reportData(float temperatureReading, float humidityReading){
  // Now we can publish stuff!
  Serial.print(F("\nSending temperature val "));
  Serial.print("...");
  if (! temperature.publish(temperatureReading)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  Serial.print(F("\nSending humidity val "));
  Serial.print("...");
  if (! humidity.publish(humidityReading)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  delay(50);//NEEDED if we sleep next
}

void connect() {
  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  
  unsigned long wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
  // Check to see if
  if (WiFi.status() == WL_CONNECT_FAILED) {
    Serial.println("Failed to connect to WiFi. Please verify credentials: ");
    delay(10000);
  }

   delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
}

void loop() {
}

void sampleTimeCallback(uint32_t seconds) {
  Serial.print("Sample Rate change: ");
  Serial.println(seconds);
  if (seconds > 1){
    sleepTime = seconds * 1e6;
  }
  Serial.print("New Sleep Time: ");
  Serial.println(sleepTime);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
