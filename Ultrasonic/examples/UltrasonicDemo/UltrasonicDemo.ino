/*
 * Show the distance in centimeters and in inches on Serial Monitor
 The circuit:
 * Connect 5v and GND, Trig to D12 and Echo to D13
 * modified 29 Oct 2013
 * by Hemal Chevli - black-electronics.com
*/
#include "Ultrasonic.h"
Ultrasonic ultrasonic(12,13); //Ultrasonic ultrasonic(Trig,Echo);

void setup() {
Serial.begin(9600);
}

void loop()
{
  Serial.print(ultrasonic.Ranging(CM)); 
  Serial.print("cm");
  Serial.print("    ");
  Serial.print(ultrasonic.Ranging(INC)); 
  Serial.println("in");
  
    
  delay(100);
}




