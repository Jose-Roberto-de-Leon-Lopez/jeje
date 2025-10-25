#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

// Configuración LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servomotor 
Servo puertaServo;
const int pinServo = 9;
const int pinBotonPuerta = 8;
const int pinBotonRotarModos = 13;
bool puertaAbierta = false;

// Motor DC (Ventilador)
const int pinVentilador = 10;

// LEDs RGB - SOLO 3 PINES PARA TODOS LOS LEDS
const int pinLEDRojo = 2;
const int pinLEDVerde = 3;  
const int pinLEDAzul = 4;

// LED RGB de estado
const int pinLEDEstadoR = 5;
const int pinLEDEstadoG = 6;
const int pinLEDEstadoB = 7;

// Módulo Bluetooth
SoftwareSerial bluetooth(11, 12);

String comandoBluetooth = "";

// Estados del sistema
enum Modo { MODO_FIESTA, MODO_RELAJADO, MODO_NOCHE, ENCENDER_TODO, APAGAR_TODO };
Modo modoActual = MODO_RELAJADO;

// Variables para modo fiesta
unsigned long tiempoAnteriorFiesta = 0;
const unsigned long intervaloFiesta = 500;
int colorFiestaActual = 0;

// Buffer para archivo .org
String lineaArchivo = "";
boolean cargandoArchivo = false;

// Variables para desplazamiento LCD
String linea1Actual = "";
String linea2Actual = "";
unsigned long tiempoUltimoDesplazamiento = 0;
const unsigned long intervaloDesplazamiento = 400; // 400ms entre desplazamientos
int posicionDesplazamiento = 0;
boolean necesitaDesplazamiento = false;

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
  
  lcd.init();
  lcd.backlight();
  
  // Mensaje inicial centrado
  lcd.setCursor(0, 0);
  lcd.print("  Sistema Activo ");
  lcd.setCursor(0, 1);
  lcd.print(" Esperando Cmd. ");
  
  pinMode(pinLEDRojo, OUTPUT);
  pinMode(pinLEDVerde, OUTPUT);
  pinMode(pinLEDAzul, OUTPUT);
  setLEDsGrupo(LOW, LOW, LOW);
  
  pinMode(pinLEDEstadoR, OUTPUT);
  pinMode(pinLEDEstadoG, OUTPUT);
  pinMode(pinLEDEstadoB, OUTPUT);
  setLEDEstado(0, 0, 1);
  
  pinMode(pinVentilador, OUTPUT);
  digitalWrite(pinVentilador, LOW);
  
  puertaServo.attach(pinServo);
  pinMode(pinBotonPuerta, INPUT_PULLUP);
  pinMode(pinBotonRotarModos, INPUT_PULLUP);
  
  cerrarPuerta();
  cambiarModo(MODO_RELAJADO);
  
  delay(1000);
}

void loop() {
  if (bluetooth.available()) {
    procesarComandoBluetooth();
  }
  
  if (Serial.available() && !cargandoArchivo) {
    procesarArchivoORG();
  }
  
  controlarBotonPuerta();
  controlarBotonRotarModos();
  
  // Controlar desplazamiento del LCD
  if (necesitaDesplazamiento) {
    controlarDesplazamientoLCD();
  }
  
  switch (modoActual) {
    case MODO_FIESTA:
      ejecutarModoFiesta();
      break;
    case MODO_RELAJADO:
    case MODO_NOCHE:
    case ENCENDER_TODO:
    case APAGAR_TODO:
      break;
  }
  
  delay(100);
}

// NUEVA FUNCIÓN: Controlar desplazamiento de texto
void controlarDesplazamientoLCD() {
  unsigned long tiempoActual = millis();
  
  if (tiempoActual - tiempoUltimoDesplazamiento >= intervaloDesplazamiento) {
    tiempoUltimoDesplazamiento = tiempoActual;
    
    // Desplazar línea 1 si es necesario
    if (linea1Actual.length() > 16) {
      String textoDesplazado = desplazarTexto(linea1Actual, posicionDesplazamiento);
      lcd.setCursor(0, 0);
      lcd.print(textoDesplazado);
    }
    
    // Desplazar línea 2 si es necesario
    if (linea2Actual.length() > 16) {
      String textoDesplazado = desplazarTexto(linea2Actual, posicionDesplazamiento);
      lcd.setCursor(0, 1);
      lcd.print(textoDesplazado);
    }
    
    posicionDesplazamiento++;
    
    // Reiniciar desplazamiento cuando llega al final
    int maxLongitud = max(linea1Actual.length(), linea2Actual.length());
    if (posicionDesplazamiento > maxLongitud + 5) {
      posicionDesplazamiento = 0;
      // Mostrar texto completo antes de reiniciar
      delay(1000);
    }
  }
}

// NUEVA FUNCIÓN: Desplazar texto
String desplazarTexto(String texto, int posicion) {
  int longitud = texto.length();
  if (longitud <= 16) {
    return texto;
  }
  
  String resultado = "";
  for (int i = 0; i < 16; i++) {
    int indice = (posicion + i) % (longitud + 5); // +5 para pausa al final
    if (indice < longitud) {
      resultado += texto[indice];
    } else {
      resultado += " "; // Espacios en blanco al final
    }
  }
  return resultado;
}

// NUEVA FUNCIÓN: Actualizar LCD con desplazamiento
void actualizarLCD(String linea1, String linea2) {
  linea1Actual = linea1;
  linea2Actual = linea2;
  
  // Verificar si necesita desplazamiento
  necesitaDesplazamiento = (linea1.length() > 16 || linea2.length() > 16);
  posicionDesplazamiento = 0;
  
  // Mostrar texto inicial
  lcd.clear();
  
  if (linea1.length() <= 16) {
    lcd.setCursor(0, 0);
    lcd.print(linea1);
  } else {
    lcd.setCursor(0, 0);
    lcd.print(linea1.substring(0, 16));
  }
  
  if (linea2.length() <= 16) {
    lcd.setCursor(0, 1);
    lcd.print(linea2);
  } else {
    lcd.setCursor(0, 1);
    lcd.print(linea2.substring(0, 16));
  }
  
  tiempoUltimoDesplazamiento = millis();
}

void setLEDsGrupo(int r, int g, int b) {
  digitalWrite(pinLEDRojo, r);
  digitalWrite(pinLEDVerde, g);
  digitalWrite(pinLEDAzul, b);
}

void setLEDEstado(int r, int g, int b) {
  digitalWrite(pinLEDEstadoR, r);
  digitalWrite(pinLEDEstadoG, g);
  digitalWrite(pinLEDEstadoB, b);
}

void parpadearLEDEstado(int r, int g, int b, int veces) {
  for (int i = 0; i < veces; i++) {
    setLEDEstado(r, g, b);
    delay(200);
    setLEDEstado(0, 0, 1);
    delay(200);
  }
}

void ejecutarModoFiesta() {
  unsigned long tiempoActual = millis();
  
  if (tiempoActual - tiempoAnteriorFiesta >= intervaloFiesta) {
    tiempoAnteriorFiesta = tiempoActual;
    
    colorFiestaActual = (colorFiestaActual + 1) % 3;
    
    switch (colorFiestaActual) {
      case 0:
        setLEDsGrupo(HIGH, LOW, LOW);
        break;
      case 1:
        setLEDsGrupo(LOW, HIGH, LOW);
        break;
      case 2:
        setLEDsGrupo(LOW, LOW, HIGH);
        break;
    }
  }
}

void abrirPuerta() {
  puertaServo.write(90);
  puertaAbierta = true;
  delay(500);
}

void cerrarPuerta() {
  puertaServo.write(0);
  puertaAbierta = false;
  delay(500);
}

void controlarBotonPuerta() {
  static unsigned long ultimoTiempoBoton = 0;
  const unsigned long debounceDelay = 300;
  
  if (millis() - ultimoTiempoBoton > debounceDelay) {
    if (digitalRead(pinBotonPuerta) == LOW) {
      if (puertaAbierta) {
        cerrarPuerta();
      } else {
        abrirPuerta();
      }
      ultimoTiempoBoton = millis();
    }
  }
}

void controlarBotonRotarModos() {
  static unsigned long ultimoTiempoRotar = 0;
  const unsigned long debounceDelay = 500;
  
  if (millis() - ultimoTiempoRotar > debounceDelay) {
    if (digitalRead(pinBotonRotarModos) == LOW) {
      rotarAlSiguienteModo();
      ultimoTiempoRotar = millis();
    }
  }
}

void rotarAlSiguienteModo() {
  Modo siguienteModo;
  
  switch (modoActual) {
    case MODO_FIESTA:
      siguienteModo = MODO_RELAJADO;
      break;
    case MODO_RELAJADO:
      siguienteModo = MODO_NOCHE;
      break;
    case MODO_NOCHE:
      siguienteModo = ENCENDER_TODO;
      break;
    case ENCENDER_TODO:
      siguienteModo = APAGAR_TODO;
      break;
    case APAGAR_TODO:
      siguienteModo = MODO_FIESTA;
      break;
    default:
      siguienteModo = MODO_FIESTA;
  }
  
  cambiarModo(siguienteModo);
  bluetooth.print("Modo rotado a: ");
  
  switch (siguienteModo) {
    case MODO_FIESTA: bluetooth.println("FIESTA"); break;
    case MODO_RELAJADO: bluetooth.println("RELAJADO"); break;
    case MODO_NOCHE: bluetooth.println("NOCHE"); break;
    case ENCENDER_TODO: bluetooth.println("ENCENDER_TODO"); break;
    case APAGAR_TODO: bluetooth.println("APAGAR_TODO"); break;
  }
}

void procesarComandoBluetooth() {
  while (bluetooth.available()) {
    char c = bluetooth.read();
    if (c == '\n') {
      comandoBluetooth.trim();
      ejecutarComando(comandoBluetooth);
      comandoBluetooth = "";
    } else {
      comandoBluetooth += c;
    }
  }
}

void ejecutarComando(String comando) {
  comando.toLowerCase();
  
  if (comando == "modo_fiesta") {
    cambiarModo(MODO_FIESTA);
    bluetooth.println("Modo FIESTA activado");
  } else if (comando == "modo_relajado") {
    cambiarModo(MODO_RELAJADO);
    bluetooth.println("Modo RELAJADO activado");
  } else if (comando == "modo_noche") {
    cambiarModo(MODO_NOCHE);
    bluetooth.println("Modo NOCHE activado");
  } else if (comando == "encender_todo") {
    cambiarModo(ENCENDER_TODO);
    bluetooth.println("Todos los dispositivos ENCENDIDOS");
  } else if (comando == "apagar_todo") {
    cambiarModo(APAGAR_TODO);
    bluetooth.println("Todos los dispositivos APAGADOS");
  } else if (comando == "estado") {
    enviarEstadoActual();
  } else {
    bluetooth.println("ERROR: Comando no reconocido");
    setLEDEstado(1, 0, 0);
    delay(1000);
    setLEDEstado(0, 0, 1);
  }
}

void procesarArchivoORG() {
  cargandoArchivo = true;
  actualizarLCD("Cargando archivo", ".org desde PC...");
  
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      lineaArchivo.trim();
      if (!procesarLineaORG(lineaArchivo)) {
        actualizarLCD("Error en archivo", ".org - Verifique");
        setLEDEstado(1, 0, 0);
        cargandoArchivo = false;
        return;
      }
      lineaArchivo = "";
    } else {
      lineaArchivo += c;
    }
    delay(10);
  }
  
  actualizarLCD("Configuracion", "guardada con exito!");
  parpadearLEDEstado(0, 1, 0, 3);
  cargandoArchivo = false;
}

bool procesarLineaORG(String linea) {
  if (linea.startsWith("//") || linea.length() == 0) {
    return true;
  }
  
  if (linea == "conf_in1") {
    return true;
  } else if (linea == "confrin") {
    return true;
  } else if (linea == "modo_fiesta") {
    EEPROM.write(0, MODO_FIESTA);
    return true;
  } else if (linea == "modo_relajado") {
    EEPROM.write(1, MODO_RELAJADO);
    return true;
  } else if (linea == "modo_noche") {
    EEPROM.write(2, MODO_NOCHE);
    return true;
  } else if (linea == "encender_todo") {
    EEPROM.write(3, ENCENDER_TODO);
    return true;
  } else if (linea == "apagar_todo") {
    EEPROM.write(4, APAGAR_TODO);
    return true;
  }
  
  return false;
}

void cambiarModo(Modo nuevoModo) {
  modoActual = nuevoModo;
  
  switch (modoActual) {
    case MODO_FIESTA:
      digitalWrite(pinVentilador, HIGH);
      setLEDsGrupo(LOW, LOW, LOW);
      actualizarLCD("Modo: FIESTA", "Vent:ON LED:Alternando");
      break;
      
    case MODO_RELAJADO:
      digitalWrite(pinVentilador, LOW);
      setLEDsGrupo(LOW, LOW, LOW);
      actualizarLCD("Modo: RELAJADO", "Vent:OFF LED:OFF");
      break;
      
    case MODO_NOCHE:
      digitalWrite(pinVentilador, LOW);
      setLEDsGrupo(LOW, LOW, LOW);
      actualizarLCD("Modo: NOCHE", "Vent:OFF LED:OFF");
      break;
      
    case ENCENDER_TODO:
      digitalWrite(pinVentilador, HIGH);
      setLEDsGrupo(HIGH, HIGH, HIGH);
      actualizarLCD("LEDS: ENCENDIDOS", "Ventilador: ACTIVADO");
      break;
      
    case APAGAR_TODO:
      digitalWrite(pinVentilador, LOW);
      setLEDsGrupo(LOW, LOW, LOW);
      actualizarLCD("LEDS: APAGADOS", "Ventilador: DESACTIVADO");
      break;
  }
}

void enviarEstadoActual() {
  bluetooth.print("Modo actual: ");
  switch (modoActual) {
    case MODO_FIESTA: bluetooth.println("FIESTA"); break;
    case MODO_RELAJADO: bluetooth.println("RELAJADO"); break;
    case MODO_NOCHE: bluetooth.println("NOCHE"); break;
    case ENCENDER_TODO: bluetooth.println("ENCENDER_TODO"); break;
    case APAGAR_TODO: bluetooth.println("APAGAR_TODO"); break;
  }
  bluetooth.print("Puerta: ");
  bluetooth.println(puertaAbierta ? "ABIERTA" : "CERRADA");
  bluetooth.print("Ventilador: ");
  bluetooth.println(digitalRead(pinVentilador) ? "ON" : "OFF");
}