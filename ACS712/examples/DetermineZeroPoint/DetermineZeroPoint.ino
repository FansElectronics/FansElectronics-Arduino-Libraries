#include "ACS712.h"

/*
  This example shows how to determine zero point of your sensor
*/

// We have 30 amps version sensor connected to A1 pin of arduino
// Replace with your version if necessary
ACS712 sensor(ACS712_30A, A1);

void setup() {
  Serial.begin(9600);

  float zero_point = sensor.calibrate();
  
  Serial.print("Zero point for this sensor is ");
  Serial.println(zero_point);
}

void loop() {}
