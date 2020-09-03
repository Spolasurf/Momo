/*
  Input Pull-up Serial

  This example demonstrates the use of pinMode(INPUT_PULLUP). It reads a digital
  input on pin 2 and prints the results to the Serial Monitor.

  The circuit:
  - momentary switch attached from pin 2 to ground
  - built-in LED on pin 13

  Unlike pinMode(INPUT), there is no pull-down resistor necessary. An internal
  20K-ohm resistor is pulled to 5V. This configuration causes the input to read
  HIGH when the switch is open, and LOW when it is closed.

  created 14 Mar 2012
  by Scott Fitzgerald

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/InputPullupSerial
*/


void setup() {
  //start serial connection
  Serial.begin(115200);
  //configure pin 2 as an input and enable the internal pull-up resistor
  pinMode(5, OUTPUT);

}

void loop() {
    digitalWrite(5, LOW);
  delay(123);
    digitalWrite(5, HIGH);
  delay(37);
    digitalWrite(5, LOW);
  delay(24);
    digitalWrite(5, HIGH);
  delay(12);
    digitalWrite(5, LOW);
  delay(13);
    digitalWrite(5, HIGH);
  delay(24);
    digitalWrite(5, LOW);
  delay(12);
    digitalWrite(5, HIGH);
  delay(37);
    digitalWrite(5, LOW);
  delay(25);
    digitalWrite(5, HIGH);
  delay(37);
    digitalWrite(5, LOW);
  delay(24);
    digitalWrite(5, HIGH);
  delay(25);
}
