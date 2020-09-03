void setup() {
  Serial.begin(115200);
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