int pinIn = 8;

void setup() {
  Serial.begin(115200);
  pinMode(pinIn, INPUT);   // Setzt den Digitalpin 7 als Intputpin
}

void loop() {
  int value = 9999;
  value = digitalRead(pinIn);  // Liest den Inputpin
  Serial.print(millis());
  Serial.print(": ");
  Serial.println(value);
}
