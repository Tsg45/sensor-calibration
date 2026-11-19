#include <HX711.h>

// ===== Wiring (ESP32) =====
// VCC -> 3.3V (แนะนำกับ ESP32)
// GND -> GND
// DT  -> GPIO23
// SCK -> GPIO22

#define DOUT_PIN  23
#define SCK_PIN   22

// ปรับตามคาลิเบรตของคุณ (ค่าอาจติดลบได้ ขึ้นกับการเดินสาย/loadcell)
float calibration_factor = -2200000.0f;

HX711 scale;  // <<--- ไม่มีพารามิเตอร์

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("HX711 Calibrating...");

  scale.begin(DOUT_PIN, SCK_PIN);     // <<--- ต้องเรียก begin()
  scale.set_scale(calibration_factor); 
  scale.tare(15);                      // ชั่งศูนย์ (อ่านเฉลี่ย 15 ครั้ง)

  if (!scale.is_ready()) {
    Serial.println("ERROR: HX711 not ready. ตรวจสายไฟ/ไฟเลี้ยง/พิน");
  } else {
    Serial.println("OK Start:");
  }
}

void loop() {
  // อ่านแบบถัวเฉลี่ยให้ค่านิ่งขึ้น
  float kg = scale.get_units(10);      // 10 ตัวอย่าง
  Serial.print("Reading: ");
  Serial.print(kg, 2);
  Serial.println(" kg");
  delay(250);
}
