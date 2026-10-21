#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// DEFINICIÓN DE PINES (ESP32 de 30 pines)
// ==========================================

#define TRIG_PIN 5
#define ECHO_PIN 18   // en físico, con divisor de voltaje 1k + 2k

#define SDA_PIN 21
#define SCL_PIN 22

#define LED1 26       // LED 1: subiendo agua (llenando)
#define LED2 27       // LED 2: nivel alto

#define BTN1 32
#define BTN2 33
#define BTN3 25

// ==========================================
// CALIBRACIÓN DEL TANQUE (en cm)
// ==========================================

#define DIST_LLENO 7.0    // 7 cm  -> 100%
#define DIST_VACIO 19.0   // 19 cm -> 0%

// Umbrales de los LEDs (en porcentaje)
#define PORC_BAJO 20      // por debajo: se enciende LED1 (subiendo agua)
#define PORC_ALTO 90      // por encima: se enciende LED2 (nivel alto)

// ==========================================
// CONFIGURACIÓN DEL LCD
// ==========================================

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// ==========================================
// VARIABLES
// ==========================================

long tiempo;
float distancia;
float porcentaje;

// ==========================================
// CONFIGURACIÓN INICIAL
// ==========================================

void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Control de nivel");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  delay(2000);
  lcd.clear();
}

// ==========================================
// PROGRAMA PRINCIPAL
// ==========================================

void loop() {

  // Pulso de disparo
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Medición
  tiempo = pulseIn(ECHO_PIN, HIGH, 30000);
  distancia = (tiempo * 0.0343) / 2.0;

  // ------------------------------------------
  // Conversión a porcentaje (7 cm = 100%, 19 cm = 0%)
  // ------------------------------------------

  porcentaje = (DIST_VACIO - distancia) * 100.0 / (DIST_VACIO - DIST_LLENO);

  if (porcentaje > 100.0) porcentaje = 100.0;
  if (porcentaje < 0.0)   porcentaje = 0.0;

  // ------------------------------------------
  // Lógica de LEDs (solo si la lectura es válida)
  // ------------------------------------------

  if (tiempo != 0) {

    if (porcentaje <= PORC_BAJO) {
      // Nivel bajo: se está llenando
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
    }
    else if (porcentaje >= PORC_ALTO) {
      // Nivel alto
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
    }
    // Entre ambos umbrales: se mantiene el estado anterior
  }

  // ------------------------------------------
  // LCD
  // ------------------------------------------

  lcd.setCursor(0, 0);
  lcd.print("Nivel:          ");
  lcd.setCursor(7, 0);

  if (tiempo == 0) {
    lcd.print("---");
  } else {
    lcd.print((int)porcentaje);
    lcd.print("%");
  }

  lcd.setCursor(0, 1);
  if (digitalRead(LED1)) {
    lcd.print("Subiendo agua   ");
  } else if (digitalRead(LED2)) {
    lcd.print("Nivel alto      ");
  } else {
    lcd.print("Esperando...    ");
  }

  // ------------------------------------------
  // Monitor serial
  // ------------------------------------------

  Serial.print("Distancia: ");
  if (tiempo == 0) {
    Serial.println("Fuera de rango");
  } else {
    Serial.print(distancia, 1);
    Serial.print(" cm  |  Nivel: ");
    Serial.print((int)porcentaje);
    Serial.println("%");
  }

  delay(300);
}
