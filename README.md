# IoT Temperature and Humidity Monitoring System

### Description
End-to-end IoT environmental monitoring system using Arduino UNO WiFi Rev2, DHT22, MQTT, HiveMQ Cloud, Node-RED, and MongoDB.
MQTT was used as the communication protocol between the Arduino and HiveMQ Cloud. Temperature and humidity measurements are published to separate MQTT topics using numeric payloads at a 5-second interval. The MQTT connection is secured using TLS over port 8883.
