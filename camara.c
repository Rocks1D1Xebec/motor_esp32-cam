#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>

const char* Nombre = "Rocks"; 
const char* Contrasena = "sombras199";

// CAMBIO: Pin para el motor. 
// Nota: El pin 4 es el Flash. Si usas un motor externo, te recomiendo el pin 12 o 13.
// Usaremos el 12 para este ejemplo.
const int pinMotor = 12; 
const int resolucion = 8; 

WebServer server(80);

static auto fixedRes = esp32cam::Resolution::find(600, 400);

void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    server.send(503, "", "");
    return;
  }
  
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void manejarControlMotor() {
  if (server.hasArg("potencia")) {
    // Recibe el número (0-255) enviado desde Python
    int velocidad = server.arg("potencia").toInt();
    
    // Aplicamos la velocidad al motor asegurando el rango
    ledcWrite(pinMotor, constrain(velocidad, 0, 255));
    
    server.send(200, "text/plain", "OK");
  }
}

void setup() {
  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(fixedRes);
  cfg.setBufferCount(2);
  cfg.setJpeg(80);
  Camera.begin(cfg);

  // Configuración del PWM para el Motor
  // Usamos 1000Hz, que es una frecuencia estándar y estable para motores pequeños
  ledcAttach(pinMotor, 1000, resolucion);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(Nombre, Contrasena);

  server.on("/cam.jpg", serveJpg);
  server.on("/control", manejarControlMotor);
  server.begin();
}

void loop() {
  server.handleClient();
}