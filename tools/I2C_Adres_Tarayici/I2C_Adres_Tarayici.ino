// ESP8266 SH1106 I2C adres tarayici (NodeMCU / Wemos D1 mini)
// SDA: D2 (GPIO4), SCL: D1 (GPIO5). Serial Monitor: 115200 baud.
#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5);
  Wire.setClock(100000);
  delay(300);
  Serial.println("\nESP8266 I2C tarama basladi.");
}

void loop() {
  uint8_t count = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C adresi: 0x");
      if (address < 16) Serial.print('0');
      Serial.println(address, HEX);
      count++;
    }
  }
  if (count == 0) Serial.println("I2C aygit bulunamadi! Kablolari kontrol edin.");
  delay(4000);
}
