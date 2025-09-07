#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include "HS4001.h"
#include "RRH46410.h"

// === Constants ===
const unsigned long PUBLISH_INTERVAL_MS = 30000;
const size_t MQTT_BUFFER_SIZE = 1024;
const size_t JSON_BUFFER_SIZE = 1024;
const size_t TOPIC_BUFFER_SIZE = 128;
const size_t VALUE_BUFFER_SIZE = 8;

const char* TOPIC_IAQ = "iaq_sensor/iaq";
const char* TOPIC_TVOC = "iaq_sensor/tvoc";
const char* TOPIC_ETOH = "iaq_sensor/etoh";
const char* TOPIC_ECO2 = "iaq_sensor/eco2";
const char* TOPIC_REL_IAQ = "iaq_sensor/relative_iaq";
const char* TOPIC_TEMP = "iaq_sensor/temperature";
const char* TOPIC_HUMID = "iaq_sensor/humidity";

HS4001 hs4001_sensor;
RRH46410 rrh46410_sensor;

WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);

bool discovery_published = false;


void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect("IAQSensor", SECRET_MQTT_USER, SECRET_MQTT_PASS)) {
      Serial.println("connected");
      // Publish discovery messages only once per boot
      if (!discovery_published) {
        publish_discovery_messages();
        discovery_published = true;
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // while (!Serial) {
  //   delay(10);
  // }
  //
  // Connect to WiFi
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // Setup MQTT
  mqtt_client.setBufferSize(1024);
  mqtt_client.setServer(SECRET_MQTT_SERVER, SECRET_MQTT_PORT);

  Wire.begin();

  if (!hs4001_sensor.begin()) {
    Serial.println("Failed to initialize HS4001 sensor!");
    while (1);
  }

  if (rrh46410_sensor.begin() != 0) {
    Serial.println("Failed to initialize RRH46410 sensor!");
    while (1);
  }

  // Set the IAQ sensor to 2nd generation IAQ mode
  rrh46410_sensor.setOpmode(OPMODE_IAQ_2ND_GEN);
}

void publish_discovery_messages() {
  publish_discovery_message("temperature", "IAQ Temperature", "°C", "temperature");
  publish_discovery_message("humidity", "IAQ Humidity", "%", "humidity");
  publish_discovery_message("iaq", "IAQ Index", "\u200B", NULL);
  publish_discovery_message("tvoc", "IAQ TVOC", "mg/m3", NULL);
  publish_discovery_message("etoh", "IAQ eTOH", "ppm", NULL);
  publish_discovery_message("eco2", "IAQ eCO2", "ppm", "carbon_dioxide");
  publish_discovery_message("relative_iaq", "Relative IAQ", "\u200B", NULL);
}

void publish_discovery_message(const char* object_id, const char* name, const char* unit_of_measurement, const char* device_class) {
  JsonDocument json_doc;
  json_doc.clear();
  char json_buffer[JSON_BUFFER_SIZE];

  JsonObject device = json_doc.createNestedObject("device");
  device["identifiers"] = "iaq_sensor_esp32c3";
  device["name"] = "IAQ Sensor";
  device["manufacturer"] = "Seeed Studio";
  device["model"] = "XIAO ESP32-C3";

  json_doc["name"] = name;
  char unique_id[64];
  sprintf(unique_id, "iaq_sensor_%s", object_id);
  json_doc["unique_id"] = unique_id;

  char state_topic[TOPIC_BUFFER_SIZE];
  sprintf(state_topic, "iaq_sensor/%s", object_id);
  json_doc["state_topic"] = state_topic;

  if (unit_of_measurement) {
    json_doc["unit_of_measurement"] = unit_of_measurement;
  }
  if (device_class) {
    json_doc["device_class"] = device_class;
  }

  char config_topic[TOPIC_BUFFER_SIZE];
  sprintf(config_topic, "homeassistant/sensor/iaq_sensor_%s/config", object_id);

  serializeJson(json_doc, json_buffer);
  mqtt_client.publish(config_topic, json_buffer, true);
}

// --- Sensor MQTT Publishing Functions ---
void publish_iaq(float value) {
  char buffer[VALUE_BUFFER_SIZE];
  dtostrf(value, 1, 1, buffer);
  mqtt_client.publish(TOPIC_IAQ, buffer);
}

void publish_tvoc(float value) {
  char buffer[VALUE_BUFFER_SIZE];
  dtostrf(value, 1, 2, buffer);
  mqtt_client.publish(TOPIC_TVOC, buffer);
}

void publish_etoh(float value) {
  char buffer[VALUE_BUFFER_SIZE];
  dtostrf(value, 1, 2, buffer);
  mqtt_client.publish(TOPIC_ETOH, buffer);
}

void publish_eco2(int value) {
  char buffer[VALUE_BUFFER_SIZE];
  itoa(value, buffer, 10);
  mqtt_client.publish(TOPIC_ECO2, buffer);
}

void publish_relative_iaq(float value) {
  char buffer[VALUE_BUFFER_SIZE];
  dtostrf(value, 1, 0, buffer);
  mqtt_client.publish(TOPIC_REL_IAQ, buffer);
}

void publish_temperature(float value) {
  char buffer[VALUE_BUFFER_SIZE];
  dtostrf(value, 1, 2, buffer);
  mqtt_client.publish(TOPIC_TEMP, buffer);
}

void publish_humidity(float value) {
  char buffer[VALUE_BUFFER_SIZE];
  dtostrf(value, 1, 2, buffer);
  mqtt_client.publish(TOPIC_HUMID, buffer);
}

void loop() {
  static unsigned long last_publish_time = 0;
  unsigned long current_time = millis();

  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();

  if (current_time - last_publish_time >= PUBLISH_INTERVAL_MS) {
    last_publish_time = current_time;

    float temperature = hs4001_sensor.readTemperature();
    float humidity = hs4001_sensor.readHumidity();

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Pass the temperature and humidity to the IAQ sensor
    rrh46410_sensor.setEnvironmentData(temperature, humidity);

    IAQResults iaq_results_struct;
    if (rrh46410_sensor.getMeasurementResults(iaq_results_struct) == 0) {
      Serial.print("IAQ: ");
      Serial.println(iaq_results_struct.iaq);

      Serial.print("TVOC: ");
      Serial.println(iaq_results_struct.tvoc);

      Serial.print("eTOH: ");
      Serial.println(iaq_results_struct.etoh);

      Serial.print("eCO2: ");
      Serial.println(iaq_results_struct.eco2);

      Serial.print("Relative IAQ: ");
      Serial.println(iaq_results_struct.relative_iaq);

      Serial.print("Sample Counter: ");
      Serial.println(iaq_results_struct.sample_counter);

      // Publish data to MQTT topics

      // Temperature: Raw value from HS4001 is in °C. No scaling needed.
      publish_temperature(temperature);

      // Humidity: Raw value from HS4001 is in %. No scaling needed.
      publish_humidity(humidity);

      // IAQ Index: Raw value is 10x the actual value. Divide by 10.
      // The scaled value corresponds to the UBA Air Quality Levels (see datasheet Table 4):
      // 1: Very Good, 2: Good, 3: Medium, 4: Poor, 5: Bad
      publish_iaq(iaq_results_struct.iaq / 10.0);

      // TVOC: Raw value is 100x the actual value. Divide by 100 to get mg/m3.
      publish_tvoc(iaq_results_struct.tvoc / 100.0);

      // eTOH: Raw value is 100x the actual value. Divide by 100 to get ppm.
      publish_etoh(iaq_results_struct.etoh / 100.0);

      // eCO2: Raw value is in ppm. No scaling needed.
      publish_eco2(iaq_results_struct.eco2);

      // Relative IAQ: Raw value needs to be multiplied by 10.
      // The scaled value is an index from 0 to 500 (see datasheet Figure 7).
      // 100: No change in air quality.
      // > 100: Degradation in air quality (air is more polluted).
      // < 100: Improvement in air quality (air is cleaner).
      publish_relative_iaq(iaq_results_struct.relative_iaq * 10.0);

    } else {
      Serial.println("Failed to get IAQ iaq_results!");
    }

    Serial.println();
  }
}
