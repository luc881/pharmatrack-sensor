#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 3, 2, U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando display GPIO2/3...");

  display.begin();
  Serial.println("begin() OK");

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(30, 35, "HUMEDAD");
  display.sendBuffer();
  Serial.println("Listo.");
}

void loop() {}
