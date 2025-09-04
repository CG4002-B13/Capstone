#include <Arduino.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include "Wire.h"

SFE_MAX1704X lipo(MAX1704X_MAX17043);
double voltage = 0;
double soc = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) ; //wait for serial monitor to open
    Serial.println("MAX17043_Example");

  Wire.begin();
  lipo.enableDebugging();

  if (!lipo.begin()) {
    Serial.println("Where device");
  }

  lipo.quickStart();
  lipo.setThreshold(32);

}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}