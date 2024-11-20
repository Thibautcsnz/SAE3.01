#define BOUTON 16  
#define LED 15 

void setup() {
  Serial.begin(115200);
  pinMode(BOUTON, INPUT_PULLUP);
}

void loop() {
  int boutonÉtat = digitalRead(BOUTON);
  Serial.println(boutonÉtat);
  delay(500);
}
