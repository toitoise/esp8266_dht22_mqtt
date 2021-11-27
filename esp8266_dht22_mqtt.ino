//================================================================
// ESP8266 et envoie par MQTT de la temperature au serveur
// ---------------------------------------------------------------
// Gestion du mode DEEP Sleep pour une faible consommation 
// et ne pas decharger les batteries en 24H
//================================================================

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define MEASUREMENT_TIMEINTERVAL 30 // Seconds
#define DHTPIN 2                    // Pin which is connected to the DHT sensor.
#define sensor      "sensor/"
#define dht11       "dht11/"
#define dht11_temp_topic "sensor/dht11/temp"
#define dht11_hum_topic  "sensor/dht11/hum"

// Uncomment the type of sensor in use:
#define DHTTYPE DHT22       // DHT 22 (AM2302)
//#define DHTTYPE DHT21     // DHT 21 (AM2301)
//#define DHTTYPE DHT11     // DHT 11

#define DEBUG     1
#define IOBROKER  0

#if IOBROKER == 0
const int   mqtt_port     = 1883;
const char* mqtt_user     = "";
const char* mqtt_password = "";
#else
// POUR TEST LOCAL
const int   mqtt_port     = 1884;
const char* mqtt_user     = "";
const char* mqtt_password = "";
#endif

// Global Objects
WiFiClient    espClient;
PubSubClient  client_mqtt(espClient);
DHT           dht(DHTPIN, DHTTYPE);

// Global varaibles
String        clientId      = "";
// le fichier identifiants.h contient les 3 lignes ci-dessous avec les bonnes valeurs 
#include "identifiants.h"
/*
const char*   wifi_ssid     = "";
const char*   mqtt_server   = "";
const char*   wifi_password = "";
*/


//----------------------------------------------------
// Get uniq Identifier with MAC Addr
//----------------------------------------------------
String getMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  String cMac = "";
  for (int i = 0; i < 6; ++i) {
    if (mac[i] < 0x10) {
      cMac += "0";
    }
    cMac += String(mac[i], HEX);
    if (i < 5) cMac += ""; // put : or - if you want byte delimiters
  }
  cMac.toUpperCase();
  return cMac;
}

//----------------------------------------------------
// Setup WIFI connection : ssid, login/pwd
//----------------------------------------------------
void setup_wifi() {
  delay(100);
  //-----------------------------------------------------------
  // We start by connecting to a WiFi network
  //-----------------------------------------------------------
  Serial.println("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("*** Connected to the WiFi network ***");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // Display MAC
  Serial.println("MAC Adresse: ");
  Serial.println(getMacAddress());
  clientId = String("ESP8266") + String("_") + getMacAddress();
  Serial.println(clientId);
}

//----------------------------------------------------
// Used if registered to subscribtion
//----------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];
    Serial.print(receivedChar);
    //  if (receivedChar == '0')   digitalWrite(in_led, LOW);
    //  if (receivedChar == '1')   digitalWrite(in_led, HIGH);
  }
}

//----------------------------------------------------
// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
//----------------------------------------------------
void reconnect_mqtt() {
  int count=10;
  client_mqtt.disconnect();
  // Loop until we're reconnected
  while (!client_mqtt.connected() && count !=0 ) {
#if DEBUG == 1
    Serial.println("Attempting MQTT connection...");
    Serial.print("State:");Serial.println(client_mqtt.state());
#endif
    // Connect to MQTT server
    if (client_mqtt.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("*** Connected to the MQTT server ***");
      // Subscribe or resubscribe to a topic
      // In this example we do not subscribe to any topic
      // client_mqtt.subscribe("your_topic_of_interest");
    } else {
      Serial.print("failed with state ");      Serial.println(client_mqtt.state());
      client_mqtt.disconnect();
      // Wait 3 seconds before retrying
      delay(3000);
      count--;
    }
  }
}

//----------------------------------------------------
// SETUP WIFI and MQTT
//----------------------------------------------------
void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client_mqtt.setServer(mqtt_server, mqtt_port);
  // The callback function is executed when some device publishes a message to a topic
  // that your ESP8266 is subscribed to
  // client_mqtt.setCallback(callback);
  Serial.println("MQTT Address: ");
  Serial.println(mqtt_server);
  Serial.println("MQTT port: ");
  Serial.println(mqtt_port);
}

//----------------------------------------------------
// Main loop
//----------------------------------------------------
void loop() {
  // Check if WIFI still connected
  if (!client_mqtt.connected()) {
    reconnect_mqtt();
  }

  if (!client_mqtt.loop()) {
    client_mqtt.connect(clientId.c_str());
  }

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h0 = dht.readHumidity();
  //float h1 = dht.readHumidity();
  //float h2 = dht.readHumidity();
  // Read temperature as Celsius (false)
  float t0 = dht.readTemperature();
  //float t1 = dht.readTemperature();
  //float t2 = dht.readTemperature();

#if DEBUG == 1
  Serial.println("Temp:");
  Serial.println(t0);
  //Serial.println(t1);
  //Serial.println(t2);
  Serial.println("Humi:");
  Serial.println(h0);
  //Serial.println(h1);
  //Serial.println(h2);
#endif

  // Check if any reads failed and exit early (to try again).
  if (isnan(h0) || isnan(t0)) {
#if DEBUG == 1
    Serial.println("Failed to read from DHT sensor!");
#endif
    t0 = 1;
    h0 = 1;
    //return;
  }

  String temp = String(sensor) + clientId + String("/") + String(dht11) + String("temperature");
  String humi = String(sensor) + clientId + String("/") + String(dht11) + String("humidity") ;

  if (client_mqtt.publish(temp.c_str(), String(t0).c_str(), false) == true ) Serial.println("Message1 ok");
  if (client_mqtt.publish(humi.c_str(), String(h0).c_str(), false) == true ) Serial.println("Message1 ok"); 
  delay(1000);

client_mqtt.disconnect();

#define CINQ_MN 300
#define DIX_MN 600
#define DEUX_MN 120
#define TRENTE_SEC 30
  Serial.println("Go DEEP");
  ESP.deepSleep(DIX_MN * 1000000);  

}
