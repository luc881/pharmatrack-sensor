#include <U8g2lib.h>

// Display Conexion — SW I2C, GPIO 4 (SDA), GPIO 5 (SCL)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 5, 4, U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n-- Test display SW I2C GPIO4/GPIO5 --");

  display.begin();
  Serial.println("begin() OK");

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(15, 30, "SW GPIO4/5");
  display.drawStr(20, 45, "Display CON");
  display.sendBuffer();
  Serial.println("Texto enviado. Debe verse en pantalla.");
}

void loop() {}
