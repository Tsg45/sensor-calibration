/* ESP32 + ACS712 (DC only, offset calibrated)
 * - ใช้ ADC1 เท่านั้น (แนะนำ GPIO34/35/36/39)
 * - ใส่ค่า SENS_MV_PER_A ให้ตรงรุ่น (5A=185, 20A=100, 30A=66 mV/A)
 * - ใส่ค่า OFFSET_MV เป็น "ค่ากึ่งกลาง" ที่ได้จากการคาลิเบรตก่อนหน้า (mV)
 */

#define ADC_PIN              34        // ขา ADC1 ของ ESP32
#define USE_MILLIVOLTS_API   1         // 1=ใช้ analogReadMilliVolts(), 0=คำนวณเอง

// ==== ใส่ค่าที่คาลิเบรตไว้แล้วที่นี่ ====
const float SENS_MV_PER_A   = 185.0f;    // mV/A   (ตัวอย่างรุ่น 5A)
const float OFFSET_MV       = 2489.8f;   // mV     (ค่า Voffset ที่คุณวัด/คาลิเบรตมาแล้ว)
// =========================================

// การตั้งค่า ADC/การเฉลี่ย
const int   OVERSAMPLE      = 400;       // จำนวนครั้งเฉลี่ยต่อการอ่านหนึ่งค่า
const float VREF_MV         = 3300.0f;   // mV (ใช้เมื่อ USE_MILLIVOLTS_API=0)
const int   ADC_MAX         = 4095;      // 12-bit ADC (ESP32)

// ===== ค่าสำหรับการปรับเทียบ =====
// Linear: I_cal = alpha * I_raw + beta
const float alpha = 1.0506f;
const float beta  = -0.0024f;  // หน่วยเป็น "แอมป์"

// Polynomial: I_cal = a*I_raw^2 + b*I_raw + c
const float a_poly = -0.0212f;
const float b_poly =  1.112f;
const float c_poly =  -0.0296f;

// ตัดสัญญาณจิ๋วใกล้ศูนย์ (เพื่อลด noise กระดิก)
const float ZERO_DEADBAND_A = 0.005f;  // 5 mA

void setup() {
  Serial.begin(115200);
  delay(200);

  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db); // ช่วง ~0-3.3V (บน ADC1)

  Serial.println(F("ESP32 + ACS712 (DC Read, calibrated)"));
  Serial.printf("SENS=%.1f mV/A, OFFSET=%.1f mV\n\n", SENS_MV_PER_A, OFFSET_MV);
}

void loop() {
  // อ่านกระแส DC (ค่า raw ยังไม่ปรับเทียบ)
  float i_raw = readCurrentDC_A(OVERSAMPLE);

  // --- Linear calibration ---
  float i_lin = (alpha * i_raw) + beta;

  // --- Polynomial calibration ---
  float i_poly = (a_poly * i_raw * i_raw) + (b_poly * i_raw) + c_poly;

  // กำจัด noise จิ๋วๆ ให้เป็นศูนย์ (ทั้ง 3 แบบ)
  i_raw  = applyDeadband(i_raw,  ZERO_DEADBAND_A);
  i_lin  = applyDeadband(i_lin,  ZERO_DEADBAND_A);
  i_poly = applyDeadband(i_poly, ZERO_DEADBAND_A);

  // แสดงผลเทียบกัน (A และ mA)
  Serial.printf("RAW  : %7.4f A  (%7.1f mA)\n", i_raw,  i_raw  * 1000.0f);
  Serial.printf("LIN  : %7.4f A  (%7.1f mA)\n", i_lin,  i_lin  * 1000.0f);
  Serial.printf("POLY : %7.4f A  (%7.1f mA)\n", i_poly, i_poly * 1000.0f);
  Serial.println("------------------------------------------------");

  delay(500);
}

// ===== ฟังก์ชันอ่าน DC (เฉลี่ยหลายครั้งให้นิ่ง) =====
float readCurrentDC_A(int samples) {
  double acc_mV = 0.0;

  for (int i = 0; i < samples; i++) {
    float mv = readInputMilliVolts();
    acc_mV += (mv - OFFSET_MV);  // ใช้ offset ที่คาลิเบรตไว้แล้ว
    delayMicroseconds(200);      // เว้นระยะให้ sampling เรียบขึ้น
  }

  float avgDiff_mV = acc_mV / samples;         // ความต่างเฉลี่ยจากจุดกึ่งกลาง (mV)
  float currentA   = avgDiff_mV / SENS_MV_PER_A;  // A = mV / (mV/A)
  return currentA;
}

// อ่านแรงดันอินพุตเป็นมิลลิโวลต์
float readInputMilliVolts() {
#if USE_MILLIVOLTS_API
  // ต้องใช้ขา ADC1 เท่านั้น (GPIO32-39) analogReadMilliVolts() จึงจะแม่นกว่า
  int mv = analogReadMilliVolts(ADC_PIN);
  return (float)mv;
#else
  int raw = analogRead(ADC_PIN);               // 0..4095
  float mv = (raw * VREF_MV) / ADC_MAX;        // แปลงเป็น mV
  return mv;
#endif
}

// ตัดค่าเล็กๆ ใกล้ศูนย์ให้เป็นศูนย์ (ลดกระดิก)
float applyDeadband(float value, float deadband) {
  if (value > -deadband && value < deadband) return 0.0f;
  return value;
}
