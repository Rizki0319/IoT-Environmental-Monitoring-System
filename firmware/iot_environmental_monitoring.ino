#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <DHT.h>

// DHT22 SETUP
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// WI-FI SETUP
char WIFI_SSID[] = "YOUR_WIFI_SSID";          //Change with your WiFi Name
char WIFI_PASS[] = "YOUR_WIFI_PASSWORD";      //Change with your WiFi Password

// HIVEMQ CLOUD MQTT SETUP (free Serverless cluster)
const char* MQTT_HOST = "YOUR_CLUSTER_URL_HIVEMQ"; // your cluster URL from HiveMQ Cloud console

// HiveMQ Cloud MQTT over TLS
const int MQTT_PORT = 8883;

// HiveMQ Cloud credentials
const char* MQTT_USER = "YOUR_USERNAME";  // credential you create under Access Management
const char* MQTT_PASS = "YOUR_PASSWORD";

// MQTT TOPICS
const char* TOPIC_TEMP    = "DHT22/environment/temperature";
const char* TOPIC_HUM     = "DHT22/environment/humidity";
const char* TOPIC_STATUS  = "DHT22/environment/status";

// PUBLISH INTERVAL
const unsigned long PUBLISH_INTERVAL_MS = 5000;

// NETWORK CLIENTS
// ======================================================

// TLS/SSL client for HiveMQ Cloud
WiFiSSLClient sslClient;

// MQTT client using SSL client
PubSubClient mqttClient(sslClient);
unsigned long lastPublish = 0;

// CONNECT WI-FI
void connectWiFi() {
  Serial.print("Wi-Fi: Connecting to ");
  Serial.println(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(3000);
    Serial.println("Wi-Fi: retry...");
  }

  Serial.println("Wi-Fi: Connected");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// CONNECT MQTT CLOUD
bool connectMQTT() {
  Serial.println("[MQTT] Entering connectMQTT()");

  Serial.print("[MQTT] Host: ");
  Serial.println(MQTT_HOST);

  Serial.print("[MQTT] Port: ");
  Serial.println(MQTT_PORT);

  Serial.println("[MQTT] Creating Client ID...");

  String clientId = "arduino-wifi-rev2-dht22-"; // Generate a unique MQTT Client ID
  clientId += String((unsigned long)millis(), HEX);

  Serial.print("[MQTT] Client ID: ");
  Serial.println(clientId);

  Serial.println("[MQTT] Calling mqttClient.connect()...");

  bool connected = mqttClient.connect(
    clientId.c_str(),
    MQTT_USER,
    MQTT_PASS
  );

  Serial.println("[MQTT] Returned from mqttClient.connect()");

  if (connected) {
    Serial.println("[MQTT] Connected!");
    return true;
  } else {
    Serial.print("[MQTT] Failed, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

// ENSURE CONNECTIONS
void ensureConnections() {
  // Check Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi: Disconnected");
    connectWiFi();
  }

  // Check MQTT
  if (!mqttClient.connected()) {
    Serial.println("MQTT: Disconnected");
    
    // Do not publish while disconnected.
    // Immediately try reconnect.

    while (!mqttClient.connected()) {
      if (connectMQTT()) {
        break;
      }

      Serial.println("MQTT: Retry in 5 seconds...");
      delay(5000);
    }
  }
}

// SETUP
void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  Serial.println();
  Serial.println("================================");
  Serial.println("Arduino WiFi Rev2 + DHT22");
  Serial.println("HiveMQ Cloud MQTT TLS");
  Serial.println("================================");

  dht.begin();

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  Serial.println("[1] Starting Wi-Fi...");
  connectWiFi();

  Serial.println("[2] Wi-Fi connected.");
  Serial.println("[3] Waiting 3 seconds...");
  delay(3000);

  Serial.println("[4] Starting MQTT connection...");
  connectMQTT();

  Serial.println("[5] Setup completed.");
}

// LOOP
void loop() {
  // Ensure connection still available
  ensureConnections();

  // MQTT keep-alive
  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  unsigned long now = millis();

  // Publish every 5 seconds
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;

    // Do not publish if MQTT is not yet connected.
    if (!mqttClient.connected()) {
      Serial.println(
        "MQTT: Not connected, data not published"
      );
      return;
    }

    // Read DHT22
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Sensor Validation
    if (isnan(h) || isnan(t)) {
      Serial.println("DHT22: Read failed");
      mqttClient.publish(TOPIC_STATUS,"sensor_read_failed",true);
      return;
    }

    // Serial Output
    Serial.println();
    Serial.println("Data Sensor:");

    Serial.print("Temperature: ");
    Serial.print(t, 1);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(h, 1);
    Serial.println(" %");

    // Convert sensor values to strings for MQTT payloads.
    char tempStr[16];
    char humStr[16];

    dtostrf(t, 0, 1, tempStr);
    dtostrf(h, 0, 1, humStr);

    // Publish temperature
    bool okT = mqttClient.publish(TOPIC_TEMP,tempStr);

    // Publish humidity
    bool okH = mqttClient.publish(TOPIC_HUM,humStr);

    // Publish result
    Serial.println("MQTT:");
    Serial.println(okT ? "Temperature published": "Temperature publish failed");
    Serial.println(okH ? "Humidity published": "Humidity publish failed");
  }
}
