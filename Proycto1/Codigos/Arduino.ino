#include <Arduino.h>

#define LED 23
#define SW1 12
#define SW2 13
#define PRENDE_LED digitalWrite(LED,HIGH)
#define APAGA_LED digitalWrite(LED,LOW)

bool sw1_prev = HIGH;
bool sw2_prev = HIGH;

void setup() {

  Serial.begin(115200);
  Serial.println("Hello, ESP32!");

  pinMode(LED, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);

}

void loop() {

  bool sw1_now = digitalRead(SW1);
  bool sw2_now = digitalRead(SW2);

  if (sw1_now == LOW && sw1_prev == HIGH) {
      PRENDE_LED;
      Serial.println("Boton 1 pulsado");
  }

  if (sw2_now == LOW && sw2_prev == HIGH) {
      APAGA_LED;
      Serial.println("Boton 2 pulsado");
  }

  sw1_prev = sw1_now;
  sw2_prev = sw2_now;

  delay(20); // debounce ligero
}
