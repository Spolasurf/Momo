int pinIn = 8;

void setup() {
  Serial.begin(115200);
  pinMode(pinIn, INPUT);
}

void loop() {
  int value = 9999;
  value = digitalRead(pinIn);  // Liest den Input Pin
  Serial.print(millis());
  Serial.print(": ");
  Serial.println(value);
}
