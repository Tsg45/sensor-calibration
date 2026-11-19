#include <WiFi.h>
#include <PubSubClient.h>

/* ====== 1) กรอกค่าให้ถูกต้อง ====== */
const char* WIFI_SSID     = "Kwan";
const char* WIFI_PASSWORD = "kwangod2549";

const char* MQTT_HOST = "test.mosquitto.org";  // เปลี่ยนเป็น broker ของคุณได้
const uint16_t MQTT_PORT = 1883;
const char* TOPIC_PUB = "natee/esp32/heartbeat"; // หัวข้อที่จะส่ง "1"

/* ====== อ็อบเจกต์เครือข่าย/MQTT ====== */
WiFiClient espClient;
PubSubClient mqtt(espClient);

/* ====== ตั้งเวลา ====== */
unsigned long lastMs = 0;
const unsigned long INTERVAL_MS = 1000; // 1 วินาที

/* ====== ฟังก์ชันช่วยต่อ WiFi / MQTT ====== */
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi OK, IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  while (!mqtt.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed rc=");
      Serial.print(mqtt.state());
      Serial.println(" retry in 2s");
      delay(2000);
    }
  }
}

/* ====== Arduino setup/loop ====== */
void setup() {
  Serial.begin(115200);
  connectWiFi();
  connectMQTT();
}

void loop() {
  // รักษาการเชื่อมต่อ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // ส่ง "1" ทุก 1 วินาที
  unsigned long now = millis();
  if (now - lastMs >= INTERVAL_MS) {
    lastMs = now;
    const char* payload = "1"; // ข้อความที่ต้องการส่ง
    bool ok = mqtt.publish(TOPIC_PUB, payload, false); // retain=false
    Serial.print("PUB ");
    Serial.print(TOPIC_PUB);
    Serial.print(" -> ");
    Serial.println(ok ? payload : "FAILED");
  }
}
