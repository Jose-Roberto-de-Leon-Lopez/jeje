const int inputPin = 2;       // Señal de contraseña correcta
const int motorHorario = 3;   // Sentido horario
const int motorAntihorario = 4; // Sentido antihorario
const int contadorAsc = 5;    // Pulso para contador ascendente

void setup() {
  pinMode(inputPin, INPUT);
  pinMode(motorHorario, OUTPUT);
  pinMode(motorAntihorario, OUTPUT);
  pinMode(contadorAsc, OUTPUT);
  
  // Inicialmente apagamos todo
  digitalWrite(motorHorario, LOW);
  digitalWrite(motorAntihorario, LOW);
  digitalWrite(contadorAsc, LOW);
}

void loop() {
  // Si recibimos señal de contraseña correcta (HIGH)
  if (digitalRead(inputPin) == HIGH) {
    
    // Delay inicial de 5 segundos
    delay(5000);
    
    // Activar motor en sentido horario
    digitalWrite(motorHorario, HIGH);
    
    // Generar 15 pulsos de 1 segundo para contador
    for (int i = 0; i < 15; i++) {
      digitalWrite(contadorAsc, HIGH); // Pulso HIGH
      delay(50);                       // Duración del pulso (50 ms)
      digitalWrite(contadorAsc, LOW);  // Bajamos a LOW
      delay(950);                      // Esperamos 950 ms (total 1 segundo)
    }
    
    // Apagar motor horario
    digitalWrite(motorHorario, LOW);
    
    // Pausa de 3 segundos
    delay(3000);
    
    // Activar motor en sentido antihorario
    digitalWrite(motorAntihorario, HIGH);
    
    // Mantener encendido por 10 segundos
    delay(10000);
    
    // Apagar motor antihorario
    digitalWrite(motorAntihorario, LOW);
  }
}