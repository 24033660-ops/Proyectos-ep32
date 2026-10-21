#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Pines ----------
#define BTN_UP    32    // Botón 1: Subir ángulo
#define BTN_DOWN  33    // Botón 2: Bajar ángulo
#define SERVO_PIN 18    // Control del Servo
#define LCD_SDA   21    // LCD I2C SDA
#define LCD_SCL   22    // LCD I2C SCL
#define LCD_ADDR  0x27  // Dirección I2C LCD

// ---------- Objetos ----------
Servo servo;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- Constantes y Variables ----------
const int MIN_ANGULO = 0;
const int MAX_ANGULO = 180;

int angulo = 0; // Posición actual del servomotor (0° a 180°)

// ---------- Ajustes para Máxima Velocidad ----------
const unsigned long DEBOUNCE_MS     = 25;
const unsigned long HOLD_INICIO_MS  = 200;  // Inicia repetición rápida tras 200ms
const unsigned long HOLD_REPETIR_MS = 20;   // Incrementa cada 20ms (Avance ultra rápido)

struct Boton {
  uint8_t pin;
  bool estable;              // HIGH = suelto, LOW = presionado
  bool lecturaAnterior;
  unsigned long ultimoCambio;
  unsigned long inicioPulsacion;
  unsigned long ultimaRepeticion;
};

Boton bUp   = {BTN_UP,   HIGH, HIGH, 0, 0, 0};
Boton bDown = {BTN_DOWN, HIGH, HIGH, 0, 0, 0};

// Función de lectura con aceleración continua
bool leerBoton(Boton &b) {
  bool lectura = digitalRead(b.pin);
  unsigned long ahora = millis();
  bool accion = false;

  if (lectura != b.lecturaAnterior) {
    b.ultimoCambio = ahora;
    b.lecturaAnterior = lectura;
  }

  if ((ahora - b.ultimoCambio) > DEBOUNCE_MS && lectura != b.estable) {
    b.estable = lectura;
    if (b.estable == LOW) {          // Presión inicial
      accion = true;
      b.inicioPulsacion = ahora;
      b.ultimaRepeticion = ahora;
    }
  }

  // Repetición veloz mientras permanezca presionado
  if (b.estable == LOW &&
      (ahora - b.inicioPulsacion) > HOLD_INICIO_MS &&
      (ahora - b.ultimaRepeticion) > HOLD_REPETIR_MS) {
    accion = true;
    b.ultimaRepeticion = ahora;
  }

  return accion;
}

// ---------- Pantalla LCD ----------
void actualizarLCD() {
  char linea0[17];
  char linea1[17];

  snprintf(linea0, sizeof(linea0), " Posicion Servo ");
  snprintf(linea1, sizeof(linea1), "   %3d deg     ", angulo);

  lcd.setCursor(0, 0);
  lcd.print(linea0);
  lcd.setCursor(0, 1);
  lcd.print(linea1);
}

// ---------- Mover Servo ----------
void cambiarValor(int delta) {
  angulo = constrain(angulo + delta, MIN_ANGULO, MAX_ANGULO);
  servo.write(angulo);
  actualizarLCD();
  Serial.printf("Angulo: %d deg\n", angulo);
}

// ---------- Setup y Loop ----------
void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2400);

  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();

  servo.write(angulo);
  actualizarLCD();
}

void loop() {
  // Botón 1 (GPIO 32): Subir ángulo
  if (leerBoton(bUp)) {
    cambiarValor(+1);
  }

  // Botón 2 (GPIO 33): Bajar ángulo
  if (leerBoton(bDown)) {
    cambiarValor(-1);
  }
}
