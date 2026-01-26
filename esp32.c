const int pinMotor = 14;      // 1. Elegimos el "enchufe" (pin) número 14 para conectar el motor
const int frecuencia = 1000;  // 2. Definimos qué tan rápido va a parpadear la energía para controlar la fuerza
const int resolucion = 8;   // 3. Decidimos que usaremos una regla del 0 al 255 para medir la potencia

void setup() {
  Serial.begin(115200);   // Abrimos una línea de comunicación para que el ESP32 pueda "hablar" con la PC
  ledcAttach(pinMotor, frecuencia, resolucion);   // Le avisamos al pin 14 que se prepare para enviar ráfagas de energía (PWM)
}
void loop() {
  if (Serial.available()) {// Preguntamos constantemente: "¿Ha llegado algún mensaje por el cable USB?"
    String data = Serial.readStringUntil('\n');// Guardamos lo que nos escribieron
    
    // Si el mensaje comienza con la letra "M" de Motor...
    if (data.startsWith("M:")) {
      
      // Quitamos la "M:" y convertimos el texto restante en un número real
      int valor = data.substring(2).toInt();
      
      // Si el número es menor a 0 o mayor a 255, lo ajustamos para que no se pase
      valor = constrain(valor, 0, 255);
      
      // Finalmente, le entregamos esa cantidad exacta de energía al motor
      ledcWrite(pinMotor, valor); 
    } 
  }
}