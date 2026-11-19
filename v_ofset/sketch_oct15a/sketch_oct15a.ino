// ESP32 : Find ACS712 Voffset (mV)
#define ADC_PIN 34
#define CALI_SAMPLES 2000   // เพิ่ม/ลดได้

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);                 // 0..4095
  analogSetPinAttenuation(ADC_PIN, ADC_11db); // ~0..3.3V
  delay(200);

  double acc = 0.0;
  for (int i = 0; i < CALI_SAMPLES; i++) {
    // ใช้ API ที่แปลงเป็น mV ให้อัตโนมัติ (ADC1 เท่านั้น)
    int mv = analogReadMilliVolts(ADC_PIN);
    acc += mv;
    delayMicroseconds(200);
  }
  float vOffset_mV = acc / CALI_SAMPLES;

  Serial.println("=== ACS712 Voffset ===");
  Serial.printf("Voffset ≈ %.1f mV\n", vOffset_mV);
  Serial.println("คัดลอกค่านี้ไปตั้งเป็น OFFSET_MV ในโปรเจกต์อ่านกระแสของคุณ");
}

void loop() {
  // ว่าง
}
