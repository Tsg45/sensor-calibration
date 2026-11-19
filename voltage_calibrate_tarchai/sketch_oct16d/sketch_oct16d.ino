#define ADCpin 33

// coefficient of linear calibration (ax + b)
float alpha = 5.1841;
float beta  = 0.4662;

// coefficient of polynomial calibration (ax^2 + bx + c)
float alpha_poly = -0.2663;
float beta_poly  = 5.6623;
float gamma_poly = 0.3505;

void setup() {
  Serial.begin(9600);

  // ========= แก้แค่บรรทัดนี้ตอนอาจารย์สั่ง =========
  analogSetWidth(12);          // ← เปลี่ยนเป็น 9 / 10 / 11 / 12 ได้เลย
  // =====================================================

  analogSetAttenuation(ADC_11db);  // 0~3.3V
  pinMode(ADCpin, INPUT);
}

void loop() {
  int raw = analogRead(ADCpin);   // ค่าจาก ADC
  int bits = 12;                  // ต้องตรงกับที่ตั้งไว้ข้างบน (แก้เลขเดียวกัน)
  float VREF = 3.3;               // ปรับตามแรงดันจริงถ้ามีการวัด

  // คำนวณแรงดันจากจำนวนบิตที่ตั้ง
  float vout = raw * (VREF / ((1 << bits) - 1));

  // Linear calibration: ax + b
  float vout_lin = (alpha * vout) + beta;

  // Polynomial calibration: ax² + bx + c
  float vout_poly = (alpha_poly * (vout * vout)) + (beta_poly * vout) + gamma_poly;

  Serial.print("[width=");
  Serial.print(bits);
  Serial.print(" bit]  raw=");
  Serial.print(raw);
  Serial.print("  vout=");
  Serial.print(vout, 4);
  Serial.print(" V  |  linear=");
  Serial.print(vout_lin, 4);
  Serial.print(" V  |  polynomial=");
  Serial.print(vout_poly, 4);
  Serial.println(" V");

  delay(500);
}