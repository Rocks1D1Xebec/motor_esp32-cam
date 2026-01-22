#include <WiFi.h>      // Librería para conectarse al Wi-Fi de la cámara
#include <WebServer.h> // Librería para recibir órdenes desde Python

// Datos de la red creada por la cámara para poder unirnos
const char* ssid = "RED_ROBOT";
const char* password = "12345678";

const int pinMotor = 13;      // Pin donde conectaremos el cable del motor
const int frecuenciaPWM = 1000; // Rapidez con la que parpadea la energía (1000 veces por segundo)
const int resolucionPWM = 8;   // Usamos una escala de 0 a 255 para la potencia

WebServer server(80); // Servidor para recibir las órdenes de Python

// Esta función recibe el valor de potencia y lo aplica al motor
void recibirPotencia() {
  if (server.hasArg("valor")) { // Si en la orden viene el dato "valor"
    int potencia = server.arg("valor").toInt(); // Convertimos el texto a número
    potencia = constrain(potencia, 0, 255);     // Nos aseguramos que no pase de 255
    ledcWrite(pinMotor, potencia);              // Le damos la orden al pin de mover el motor
    server.send(200, "text/plain", "OK");       // Respondemos que todo salió bien
  }
}

void setup() {
  Serial.begin(115200);

  // Configuramos el pin del motor para que pueda variar su fuerza (PWM)
  ledcAttach(pinMotor, frecuenciaPWM, resolucionPWM);
  ledcWrite(pinMotor, 0); // Empezamos con el motor apagado

  WiFi.begin(ssid, password); // Intentamos conectarnos a la red de la cámara
  while (WiFi.status() != WL_CONNECTED) { // Mientras no se conecte...
    delay(500);
    Serial.print("."); // Imprimimos puntos para saber que sigue intentando
  }

  server.on("/motor", recibirPotencia); // Si Python dice "/motor", ejecutamos la función
  server.begin(); // Iniciamos el servidor
}

void loop() {
  server.handleClient(); // El ESP32 espera las órdenes de Python constantemente
}