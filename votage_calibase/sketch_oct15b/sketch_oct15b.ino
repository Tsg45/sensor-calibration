
#define ADCpin 33

// coefficient of linear calibraton สัมประสิทธิ์ของการปรับเทียบแบบเส้นตรง
float alpha = 5.1841;
float beta = 0.4662;

// coefficient of polynomial calibration สัมประสิทธิ์ของการปรับเทียบแบบพหุนาม
float alpha_poly = -0.2663;
float beta_poly = 5.6623;
float gamma_poly = 0.3505;


void setup() {
  Serial.begin(9600);
  analogSetWidth(12);  // Define width of bit each
  analogSetAttenuation(ADC_11db);
  pinMode(ADCpin, INPUT);
}

void loop() {

  // // ADC value from voltage sensor 
   int analog_voltage_value = analogRead(ADCpin);
  Serial.print("voltage sensor : [");
  Serial.print(analog_voltage_value);
  Serial.println("] Volts");
  delay(500);

// Calculate a Voltage from ADC
  float vout = (analog_voltage_value * 3.3) / (float)(1 << 12);
  Serial.print("[sensor] : [");
  Serial.print(vout);
  Serial.println("] Volts");
  delay(500);
  

  // //Linear Calibration voltage sensor [ax+b]
  // float voutcali = (alpha * vout) + beta;
  // Serial.print("[linear calibrate sensor] : [");
  // Serial.print(voutcali);
  // Serial.println("] Volts");
  // delay(500);

  // // Polynomial Calibration voltage sensor [ax^2+bx+c]
  // float voutcalipoly = (alpha_poly*(pow(vout,2)))+(beta_poly*vout)+gamma_poly;
  // Serial.print("[polynomial calibrate sensor] : [");
  // Serial.print(voutcalipoly);
  // Serial.println("] Volts");
  // delay(500);
}