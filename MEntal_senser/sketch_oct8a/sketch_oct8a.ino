#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 4 //กำหนดขาที่จะเชื่อมต่อ Sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
 
void setup(void) {
Serial.begin(9600);
Serial.println("Dallas Temperature IC Control Library");
sensors.begin();
}
 
void loop(void) {
sensors.requestTemperatures(); //อ่านข้อมูลจาก library
Serial.print("Temperature is: ");
Serial.print(sensors.getTempCByIndex(0)); // แสดงค่า อูณหภูมิ 
Serial.println(" *C");
delay(500);
}