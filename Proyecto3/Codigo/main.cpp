#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// ASIGNACIÓN DE PINES (ESP32)
// ==========================================
#define DS18B20_PIN 4   // Sensor de temperatura DS18B20 (GPIO 4)
#define SDA_PIN     21  // LCD I2C SDA (GPIO 21)
#define SCL_PIN     22  // LCD I2C SCL (GPIO 22)

#define BTN_1_PIN   32  // Botón 1: Incrementar (+1 °C)
#define BTN_2_PIN   33  // Botón 2: Decrementar (-1 °C)
#define BTN_3_PIN   25  // Botón 3: Seleccionar Min / Max

#define RELE_FOCO_PIN    2   // Relé 1: Foco / Calentador (GPIO 2 - D2)
#define RELE_PELTIER_PIN 15  // Relé 2: Celda Peltier / Enfriador (GPIO 15 - D15)

// ==========================================
// CONFIGURACIÓN DE LÓGICA DE RELÉS (Active LOW)
// ==========================================
// Si tu módulo se activa con LOW (lo estándar), déjalo así.
// Si tu módulo es Active HIGH, invierte estos dos valores.
#define RELE_ON  LOW   
#define RELE_OFF HIGH  

// ==========================================
// OBJETOS Y VARIABLES DE ESTADO
// ==========================================
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float temperaturaActual = 0.0;

// Umbrales de temperatura (°C)
float tempMin = 30.0; // Límite inferior para encender Foco
float tempMax = 60.0; // Límite superior para encender Peltier

// 0 = Editar Temp Mínima, 1 = Editar Temp Máxima
uint8_t limiteSeleccionado = 0; 

// Estado del Ciclo: 0 = Calentando (Foco), 1 = Enfriando (Peltier)
enum EstadoCiclo { CALENTANDO, ENFRIANDO };
EstadoCiclo estadoActual = CALENTANDO;

// Control de tiempo y antirrebote (Debounce)
unsigned long ultimaLecturaTemp = 0;
const unsigned long INTERVALO_TEMP = 1000;

bool estadoAnteriorBtn1 = HIGH;
bool estadoAnteriorBtn2 = HIGH;
bool estadoAnteriorBtn3 = HIGH;

unsigned long ultimoTiempoBoton = 0;
const unsigned long TIEMPO_DEBOUNCE = 180;

void setup() {
  Serial.begin(115200);

  // Configuración de botones con Pull-up interna
  pinMode(BTN_1_PIN, INPUT_PULLUP);
  pinMode(BTN_2_PIN, INPUT_PULLUP);
  pinMode(BTN_3_PIN, INPUT_PULLUP);

  // Configuración de relés (Inician apagados)
  pinMode(RELE_FOCO_PIN, OUTPUT);
  pinMode(RELE_PELTIER_PIN, OUTPUT);
  digitalWrite(RELE_FOCO_PIN, RELE_OFF);
  digitalWrite(RELE_PELTIER_PIN, RELE_OFF);

  // Inicializar Pantalla LCD e I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ciclo Termico");
  lcd.setCursor(0, 1);
  lcd.print("Foco / Peltier");

  // Inicializar DS18B20
  sensors.begin();
  sensors.setResolution(9);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long ahora = millis();

  // ----------------------------------------------------
  // 1. LECTURA Y CONTROL DE BOTONES (ANTIRREBOTE)
  // ----------------------------------------------------
  bool btn1Actual = digitalRead(BTN_1_PIN);
  bool btn2Actual = digitalRead(BTN_2_PIN);
  bool btn3Actual = digitalRead(BTN_3_PIN);

  if (ahora - ultimoTiempoBoton >= TIEMPO_DEBOUNCE) {
    // BOTÓN 3: Conmuta entre editar T_min o T_max
    if (btn3Actual == LOW && estadoAnteriorBtn3 == HIGH) {
      limiteSeleccionado = (limiteSeleccionado == 0) ? 1 : 0;
      ultimoTiempoBoton = ahora;
    }

    // BOTÓN 1: Aumenta la temperatura seleccionada (+1 °C)
    if (btn1Actual == LOW && estadoAnteriorBtn1 == HIGH) {
      if (limiteSeleccionado == 0) {
        if (tempMin + 1.0 < tempMax) tempMin += 1.0;
      } else {
        tempMax += 1.0;
      }
      ultimoTiempoBoton = ahora;
    }

    // BOTÓN 2: Disminuye la temperatura seleccionada (-1 °C)
    if (btn2Actual == LOW && estadoAnteriorBtn2 == HIGH) {
      if (limiteSeleccionado == 0) {
        tempMin -= 1.0;
      } else {
        if (tempMax - 1.0 > tempMin) tempMax -= 1.0;
      }
      ultimoTiempoBoton = ahora;
    }
  }

  estadoAnteriorBtn1 = btn1Actual;
  estadoAnteriorBtn2 = btn2Actual;
  estadoAnteriorBtn3 = btn3Actual;

  // ----------------------------------------------------
  // 2. LECTURA DE TEMPERATURA (Cada 1 segundo)
  // ----------------------------------------------------
  if (ahora - ultimaLecturaTemp >= INTERVALO_TEMP) {
    float tempRead = sensors.getTempCByIndex(0);
    if (tempRead > -55.0 && tempRead < 125.0) {
      temperaturaActual = tempRead;
    }
    sensors.requestTemperatures();
    ultimaLecturaTemp = ahora;

    // Reporte por puerto serie
    Serial.print("Temp: ");
    Serial.print(temperaturaActual, 1);
    Serial.print(" C | Min: ");
    Serial.print(tempMin, 0);
    Serial.print(" C | Max: ");
    Serial.print(tempMax, 0);
    Serial.print(" C | Estado: ");
    Serial.println(estadoActual == CALENTANDO ? "CALENTANDO (Foco)" : "ENFRIANDO (Peltier)");
  }

  // ----------------------------------------------------
  // 3. LÓGICA DE CONTROL POR CICLO (HISTERESIS)
  // ----------------------------------------------------
  // Evaluación del cambio de estado
  if (estadoActual == CALENTANDO) {
    // Si sobrepasa o alcanza la temp máxima, cambia a enfriar
    if (temperaturaActual >= tempMax) {
      estadoActual = ENFRIANDO;
    }
  } else if (estadoActual == ENFRIANDO) {
    // Si baja o alcanza la temp mínima, cambia a calentar
    if (temperaturaActual <= tempMin) {
      estadoActual = CALENTANDO;
    }
  }

  // Ejecución de salidas según el estado activo del ciclo
  if (estadoActual == CALENTANDO) {
    digitalWrite(RELE_FOCO_PIN, RELE_ON);     // Encender Foco
    digitalWrite(RELE_PELTIER_PIN, RELE_OFF); // Apagar Peltier
  } else {
    digitalWrite(RELE_FOCO_PIN, RELE_OFF);   // Apagar Foco
    digitalWrite(RELE_PELTIER_PIN, RELE_ON);  // Encender Peltier
  }

  // ----------------------------------------------------
  // 4. VISUALIZACIÓN EN PANTALLA LCD (16x2)
  // ----------------------------------------------------
  // Línea 0: Temperatura actual y Estado del Ciclo
  lcd.setCursor(0, 0);
  lcd.print("T:");
  if (temperaturaActual < -50) {
    lcd.print("Error   ");
  } else {
    lcd.print(temperaturaActual, 1);
    lcd.print((char)223);
    lcd.print("C ");
  }
  lcd.setCursor(11, 0);
  lcd.print("[");
  lcd.print(estadoActual == CALENTANDO ? "CAL" : "ENF");
  lcd.print("]");

  // Línea 1: Umbrales Min y Max (* indica el seleccionado para edición)
  lcd.setCursor(0, 1);
  if (limiteSeleccionado == 0) lcd.print("*"); else lcd.print(" ");
  lcd.print("Min:");
  lcd.print((int)tempMin);
  lcd.print((char)223);

  lcd.setCursor(8, 1);
  if (limiteSeleccionado == 1) lcd.print("*"); else lcd.print(" ");
  lcd.print("Max:");
  lcd.print((int)tempMax);
  lcd.print((char)223);
}
