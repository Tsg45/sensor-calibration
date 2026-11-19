#include <max6675.h>  // Adafruit: ตัวเล็ก

const int thermoSO  = 19;
const int thermoCS  = 5;
const int thermoSCK = 18;

MAX6675 tc(thermoSCK, thermoCS, thermoSO);

void setup() {
  Serial.begin(115200);
  Serial.println("MAX6675 ESP32 Test (Adafruit)");
  delay(250);
}

void loop() {
  double c = tc.readCelsius();   // <-- ใช้ readCelsius()
  if (isnan(c)) {
    Serial.println("NAN: เซนเซอร์/สายหลุดหรือขาดวงจร");
  } else {
    double f = tc.readFahrenheit();  // มีเมธอดโดยตรง
    Serial.printf("Temperature: %.2f °C / %.2f °F\n", c, f);
  }
  delay(500);
}
