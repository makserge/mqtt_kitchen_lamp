#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "***";
const char* WIFI_PASSWD = "***";

const byte LAMP_OUT = 14;

const byte MAX_WIFI_CONNECT_DELAY = 50;
const int MQTT_BUFFER_SIZE = 50;

IPAddress mqttServer(192, 168, 31, 100); // mosquitto address
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
const char* MQTT_CLIENT = "ESP8266";

const char* MQTT_TOPIC = "kitchen/lamp";

WiFiUDP udp;
WiFiClient httpClient;

PubSubClient mqttClient(httpClient, mqttServer);

void publishMqtt(String pubTopic, String payload) {
  if (mqttClient.connected()) {
    mqttClient.publish(pubTopic, (char*) payload.c_str());
  }
}

void waitForWifiConnection() {
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && (retries < MAX_WIFI_CONNECT_DELAY)) {
    retries++;
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    abort();
  }
}

void lampInit() {
  pinMode(LAMP_OUT, OUTPUT);
  analogWrite(LAMP_OUT, 255);
}

void mQttCallback(const MQTT::Publish& pub) {
  if (pub.payload_len() == 0) {
    return;
  }
  String payload = pub.payload_string();
  StaticJsonBuffer<MQTT_BUFFER_SIZE> jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  int value = root["value"];
  if (value > 255) {
    return;
  }
  analogWrite(LAMP_OUT, value);
}

void setup() {
  lampInit();
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);

  waitForWifiConnection();
  Serial.println("Wi-Fi was connected");
  analogWrite(LAMP_OUT, 0);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    abort();
    return;
  }  
  if (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT::Connect(MQTT_CLIENT).set_auth(MQTT_USER, MQTT_PASS))) {
      Serial.println("Connected to MQTT");
      mqttClient.set_callback(mQttCallback);
      mqttClient.subscribe(MQTT_TOPIC);
    }
  }
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
  delay(100);
}
