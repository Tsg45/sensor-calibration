/***** รวม MAX30105 + BPM + WiFi + MQTT (ESP32) *****/
#include <Wire.h>
#include "MAX30105.h"        // SparkFun MAX3010x library
#include "heartRate.h"       // มีฟังก์ชัน checkForBeat()
#include <WiFi.h>
#include <PubSubClient.h>

/* ====== 1) ตั้งค่า WiFi/MQTT ====== */
const char* WIFI_SSID     = "Kwan";
const char* WIFI_PASSWORD = "kwangod2549";
const char* MQTT_HOST     = "test.mosquitto.org";  // เปลี่ยนเป็น broker ของคุณได้
const uint16_t MQTT_PORT  = 1883;
const char* TOPIC_PUB     = "natee/esp32/heartbeat"; // ส่ง "1" เมื่อ Avg BPM > 90

/* ====== อ็อบเจกต์เซนเซอร์/เครือข่าย ====== */
MAX30105 particleSensor;
WiFiClient espClient;
PubSubClient mqtt(espClient);

/* ====== ตัวแปรคำนวณ BPM ====== */
const byte RATE_SIZE = 4;            // ค่าเฉลี่ยจาก 4 ครั้งล่าสุด (ปรับได้ 4–10)
byte rates[RATE_SIZE];               // เก็บ BPM เพื่อเอามาเฉลี่ย
byte rateSpot = 0;
long lastBeat = 0;                   // เวลา ms ของ beat ล่าสุด
float beatsPerMinute = 0;
int   beatAvg = 0;

/* ====== เกณฑ์และจังหวะเวลา ====== */
const int IR_FINGER_THRESHOLD = 50000;   // ต่ำกว่านี้ถือว่า “ไม่นิ้ว”
const int BPM_THRESHOLD       = 50;      // เงื่อนไขส่ง MQTT
unsigned long lastPubMs       = 0;
const unsigned long PUB_WINDOW_MS = 1000; // เช็ค/ส่งไม่เกินทุก 1 วินาที

/* ====== ฟังก์ชันเชื่อมต่อเครือข่าย ====== */
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

/* ====== ตั้งค่า MAX30105 ====== */
bool initMAX30105() {
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 not found. Check wiring/power.");
    return false;
  }

  // ค่าตั้งต้นที่ใช้งานได้ดีทั่วไป
  byte ledBrightness = 0x1F; // 0x02–0xFF (สว่างมาก = สัญญาณแรงแต่กินไฟ)
  byte sampleAverage = 4;    // 1,2,4,8,16,32
  byte ledMode = 2;          // 1 = Red, 2 = Red+IR, 3 = Red+IR+Green
  int sampleRate = 100;      // Hz: 50,100,200,400,...
  int pulseWidth = 411;      // 69,118,215,411 us
  int adcRange = 16384;      // 2048,4096,8192,16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  // ให้ไฟแดงติดอ่อน ๆ เพื่อบอกว่าทำงาน
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);  // ปิด Green

  Serial.println("Place your index finger on the sensor with steady pressure.");
  return true;
}

/* ====== Arduino setup ====== */
void setup() {
  Serial.begin(115200);
  delay(200);

  // I2C สำหรับ ESP32 (SDA=21, SCL=22 ค่า default)
  Wire.begin();

  if (!initMAX30105()) {
    while (true) { delay(1000); }  // ค้างไว้ถ้าไม่พบเซนเซอร์
  }

  connectWiFi();
  connectMQTT();
}

/* ====== Arduino loop ====== */
void loop() {
  // รักษาการเชื่อมต่อ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // อ่าน IR เพื่อตรวจจับการวางนิ้วและชีพจร
  long irValue = particleSensor.getIR();

  // ตรวจพบจังหวะหัวใจ (beat)
  if (checkForBeat(irValue) == true) {
    long nowMs = millis();
    long delta = nowMs - lastBeat;
    lastBeat = nowMs;

    if (delta > 0) {
      beatsPerMinute = 60.0 / (delta / 1000.0);
      // กรองค่าแปลก ๆ
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        // คำนวณค่าเฉลี่ยเคลื่อนที่
        int sum = 0;
        for (byte i = 0; i < RATE_SIZE; i++) sum += rates[i];
        beatAvg = sum / RATE_SIZE;
      }
    }
  }

  // แสดงสถานะบน Serial
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute, 1);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < IR_FINGER_THRESHOLD) {
    Serial.print("  (No finger?)");
  }
  Serial.println();

  // === เงื่อนไขการส่งไป Node-RED ===
  // ส่งไม่เกินทุก 1 วินาที และต้องมีนิ้ว + Avg BPM > 90
  unsigned long now = millis();
  if (now - lastPubMs >= PUB_WINDOW_MS) {
    lastPubMs = now;

    bool hasFinger = irValue >= IR_FINGER_THRESHOLD;
    if (hasFinger && beatAvg > BPM_THRESHOLD) {
      // ส่ง "1" เท่านั้นตามที่ต้องการ
      const char* payload = ">90";
      bool ok = mqtt.publish(TOPIC_PUB, payload, false);
      Serial.print("PUB ");
      Serial.print(TOPIC_PUB);
      Serial.print(" -> ");
      Serial.println(ok ? payload : "FAILED");
    } else {
      // ไม่ส่งอะไรเมื่อ <= 90 หรือไม่มีนิ้ว
      // (ถ้าอยากเห็น log ให้ uncomment บรรทัดด้านล่าง)
      // Serial.println("Not publishing (no finger or Avg<=90).");
    }
  }
}
