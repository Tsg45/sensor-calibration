/* ESP32 + ACS712 (DC Read) — Improved & Calibratable
 * สาย: OUT->GPIO34 (ADC1), VCC->5V, GND ร่วม
 * คำสั่ง Serial:
 *  - 'c' : คาลิเบรต OFFSET_MV (ทำตอน 0A)
 *  - 's' : ตั้งสเกลด้วยค่ากระแสจริงจากมิเตอร์ (หน่วย A)
 *  - 'b' : ตั้ง I_OFFSET_A = ค่าที่อ่านปัจจุบัน (ใช้ตอน 0A เพื่อชดเชยศูนย์)
 *  - 'p' : แสดงค่าพารามิเตอร์
 */

#define ADC_PIN            34
#define USE_MILLIVOLTS_API 1

// ===== ค่าตั้งต้น (แก้ได้) =====
float SENS_MV_PER_A = 174.66f;   // ปรับแล้วจาก 185 → 174.66 mV/A ให้ตรง ~207.60 mA
float OFFSET_MV     = 2533.1f;   // ใส่ค่า offset ที่คุณเคยวัด; แนะนำกด 'c' ตอน 0A ทุกครั้งที่เปิด
float I_OFFSET_A    = 0.0000f;   // ออฟเซ็ตชดเชยเป็นแอมป์ (ถ้า 0A แล้วอ่านไม่เป็น 0)

// อ่าน/เฉลี่ย
const int   OVERSAMPLE = 600;
const float VREF_MV    = 3300.0f;  // ใช้เมื่อไม่ใช้ analogReadMilliVolts()
const int   ADC_MAX    = 4095;

void setup() {
  Serial.begin(115200);
  delay(200);
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  Serial.println(F("ESP32 + ACS712 (DC) — Improved"));
  printParams();
  Serial.println(F("Tips: เปิดแบบ 0A แล้วกด 'c' เพื่อ zero offset ให้เป๊ะก่อนวัดจริง\n"));
}

void loop() {
  // คำสั่ง Serial
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') doCalOffset();   // zero at 0A
    if (c == 's' || c == 'S') doCalScale();    // scale to meter
    if (c == 'b' || c == 'B') doCalBiasA();    // bias at 0A
    if (c == 'p' || c == 'P') printParams();
  }

  // อ่านกระแส DC
  float iA = readCurrentDC_A(OVERSAMPLE);
  iA -= I_OFFSET_A;  // ชดเชยออฟเซ็ตเป็นแอมป์ (ละเอียด)

  Serial.printf("I_DC = %.4f A  (%.1f mA)\n", iA, iA * 1000.0f);
  delay(500);
}

// ====== ฟังก์ชันอ่าน DC ======
float readCurrentDC_A(int samples) {
  double acc_mV = 0.0;
  for (int i = 0; i < samples; i++) {
    float mv = readInputMilliVolts();
    acc_mV += (mv - OFFSET_MV);
    delayMicroseconds(200);
  }
  float avgDiff_mV = acc_mV / samples;
  return avgDiff_mV / SENS_MV_PER_A;  // A = mV / (mV/A)
}

// อ่านแรงดันอินพุตเป็น mV
float readInputMilliVolts() {
#if USE_MILLIVOLTS_API
  // ใช้ ADC1 เท่านั้น
  int mv = analogReadMilliVolts(ADC_PIN);
  return (float)mv;
#else
  int raw = analogRead(ADC_PIN);           // 0..4095
  return (raw * VREF_MV) / ADC_MAX;        // mV
#endif
}

// ====== คาลิเบรต OFFSET (ศูนย์ที่ 0A) ======
void doCalOffset() {
  const int N = 2000;
  double acc = 0.0;
  delay(50);
  Serial.println(F("\n[Cal Offset] ทำตอน 0A จริง..."));
  for (int i = 0; i < N; i++) {
    acc += readInputMilliVolts();
    delayMicroseconds(200);
  }
  OFFSET_MV = acc / N;
  Serial.printf("OFFSET_MV = %.1f mV\n\n", OFFSET_MV);
  printParams();
}

// ====== คาลิเบรต SCALE (ตั้งจากค่ามิเตอร์) ======
void doCalScale() {
  Serial.println(F("\n[Cal Scale] ใส่ค่ากระแสจริงจากมิเตอร์ (A) เช่น 0.2076 แล้ว Enter: "));
  while (!Serial.available()) delay(10);
  String s = Serial.readStringUntil('\n');
  s.trim();
  float i_true = s.toFloat();
  if (i_true <= 0) {
    Serial.println(F("ค่าไม่ถูกต้อง / <= 0, ยกเลิก\n"));
    return;
  }

  const int N = 1200;
  double acc_mV = 0.0;
  for (int i = 0; i < N; i++) {
    acc_mV += (readInputMilliVolts() - OFFSET_MV);
    delayMicroseconds(200);
  }
  float avgDiff_mV = acc_mV / N;

  if (fabs(avgDiff_mV) < 0.1f) {
    Serial.println(F("สัญญาณใกล้ 0 mV — ใช้กระแสอ้างอิงที่มากขึ้น\n"));
    return;
  }

  // SENS ใหม่ = mV ต่อ A
  SENS_MV_PER_A = avgDiff_mV / i_true;
  Serial.printf("SENS_MV_PER_A ใหม่ = %.3f mV/A\n\n", SENS_MV_PER_A);
  printParams();
}

// ====== ตั้งออฟเซ็ตเป็นแอมป์จากค่าที่อ่านตอน 0A ======
void doCalBiasA() {
  // อ่านค่าปัจจุบัน แล้วทำให้กลายเป็นศูนย์ด้วย I_OFFSET_A
  float i_now = readCurrentDC_A(OVERSAMPLE);
  I_OFFSET_A = i_now;  // ครั้งถัดไป iA -= I_OFFSET_A จะได้ 0
  Serial.printf("\n[Bias A] I_OFFSET_A = %.4f A (ใช้ชดเชยศูนย์ละเอียด)\n\n", I_OFFSET_A);
  printParams();
}

void printParams() {
  Serial.printf("PARAMS => SENS=%.3f mV/A, OFFSET=%.1f mV, I_OFFSET_A=%.4f A\n",
                SENS_MV_PER_A, OFFSET_MV, I_OFFSET_A);
}
