#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Test display GPIO2(SDA) / GPIO3(SCL)");

  Wire.begin(2, 3);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error 0x3C, probando 0x3D...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("No responde en ninguna direccion.");
      return;
    }
  }

  Serial.println("Display OK!");
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 24);
  display.println("HUMEDAD");
  display.display();
}

void loop() {}
