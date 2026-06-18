#include <Wire.h>
#include <U8g2lib.h>

// Display Humedad — Wire1 hardware I2C, GPIO 6 (SDA), GPIO 7 (SCL)
U8G2_SSD1306_128X64_NONAME_F_2ND_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n-- Test display Wire1 GPIO6/GPIO7 --");

  Wire1.begin(6, 7);  // SDA=6, SCL=7 — antes de display.begin()
  Serial.println("Wire1 OK");

  display.begin();
  Serial.println("begin() OK");

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(15, 30, "Wire1 GPIO6/7");
  display.drawStr(20, 45, "Display HUM");
  display.sendBuffer();
  Serial.println("Texto enviado. Debe verse en pantalla.");
}

void loop() {}
