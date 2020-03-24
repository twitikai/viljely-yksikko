
/*
 * Aeroponisen viljelyn PoC:ssa käytetyn mittausboxin lähdekoodi
 * Huomioi, että tämän tiedoston lisäksi Arduino IDE:n projektissa pitää olla tiedosto secrets.h, 
 * jossa on määriteltynä seuraavat muuttujat. Tämä ratkaisu on tehty siksi, että GitHubiin ladatussa
 * koodissa ei levitetä käyttäjätunnuksia ja salasanoja.
 * 
 * const char* ssid = "SSID OF YOUR WIFI NETWORK";
 * const char* password = "YOUR WIFI PASSWORD";
 * const char* mqtt_server = "YOUR MQTT BROKER's IP ADDRESS";
 * 
 * Koko tämä esimerkkikoodi pohjautuu voimakkaasti valmiiseen esimerkkikoodiin, eikä siitä tarkoituksella 
 * ole siivottu tälle esimerkille tarpeetonta koodia pois. Näin ollen esimerkiksi kaksisuuntaisen 
 * MQTT-viestittelyn toteuttamiseen on tässä jo esimerkkikoodia olemassa.
 * 
 * 
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

#define DHTTYPE   DHT11 
#define DHTPIN    D4
#define DHT2PIN   D3

// Update these with values suitable for your network.

const char* topicH = "aeroponics/humidity";
const char* topicT = "aeroponics/temperature";
const char* topicDBG = "aeroponics/debug";
const char* topicH2 = "aeroponics/humidity2";
const char* topicT2 = "aeroponics/temperature2";


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
DHT dht(DHTPIN,DHTTYPE);
DHT dht2(DHT2PIN,DHTTYPE);

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  dht.begin();
  dht2.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Aeroponics PoC - ";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topicDBG, "Aeroponics connected");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 10000) {
    
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "#%ld", value);

    Serial.println("Reading sensor values from sensor 1");
    float humidity = dht.readHumidity(); 
    float temperature = dht.readTemperature();

    if (isnan(temperature) || isnan(humidity)) {
       Serial.println("Failed to read from DHT sensor 1!");
      return; 
    }
    Serial.print("Humidity:");
    Serial.println(humidity);
    Serial.print("Temperature:");
    Serial.println(temperature);


    Serial.println("Reading sensor values from sensor 2");
    float humidity2 = dht2.readHumidity(); 
    float temperature2 = dht2.readTemperature();

    if (isnan(temperature) || isnan(humidity)) {
       Serial.println("Failed to read from DHT sensor 1!");
       return;
    }

    Serial.print("Humidity2:");
    Serial.println(humidity2);
    Serial.print("Temperature2:");
    Serial.println(temperature2);

    Serial.print("MQTT server connected");
    Serial.println(client.connected());

    
    Serial.print("Publish messages: ");
    client.publish(topicH, String(humidity).c_str());
    client.publish(topicT, String(temperature).c_str());
    delay(500);
    client.publish(topicH2, String(humidity2).c_str());
    client.publish(topicT2, String(temperature2).c_str());
    Serial.println("published");
  }
}
