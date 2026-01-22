#include <WebServer.h> // Librería para crear una página web interna
#include <WiFi.h>      // Librería para manejar las conexiones Wi-Fi
#include <esp32cam.h>  // Librería simplificada para la cámara

// Nombre y contraseña de la red que creará la cámara
const char* Nombre = "RED_ROBOT"; 
const char* Contrasena = "12345678";

WebServer server(80); // Creamos un servidor que escucha en el puerto 80 (el estándar)

// Configuramos la resolución: 320 píxeles de ancho por 240 de alto (pequeña para que sea rápida)
static auto res_fija = esp32cam::Resolution::find(320, 240);

// Esta función se encarga de capturar la foto y enviarla
void enviarFoto() {
  auto frame = esp32cam::capture(); // La cámara toma la foto en este instante
  if (frame == nullptr) {           // Si la foto sale vacía o falla
    server.send(503, "", "");       // Enviamos un mensaje de error
    return;
  }
  
  server.setContentLength(frame->size()); // Decimos cuánto pesa la imagen
  server.send(200, "image/jpeg");         // Decimos que lo que enviamos es una imagen JPEG
  WiFiClient cliente = server.client();   // Creamos el túnel de envío
  frame->writeTo(cliente);                // Mandamos la imagen por el túnel
}

void setup() {
  Serial.begin(115200); // Iniciamos la comunicación para ver mensajes en la PC

  using namespace esp32cam;
  Config cfg; // Creamos una configuración para la cámara
  cfg.setPins(pins::AiThinker); // Le decimos que usamos el modelo AI-Thinker (el más común)
  cfg.setResolution(res_fija);  // Le ponemos la resolución que elegimos arriba
  cfg.setBufferCount(2);        // Usamos dos memorias temporales para que el video sea fluido
  cfg.setJpeg(80);              // Calidad de la imagen al 80%
  Camera.begin(cfg);            // Encendemos la cámara con esa configuración

  WiFi.mode(WIFI_AP);           // Ponemos la placa en modo "Punto de Acceso" (como un router)
  WiFi.softAP(Nombre, Contrasena); // Iniciamos la red con el nombre y clave elegidos

  server.on("/cam.jpg", enviarFoto); // Si alguien pide "/cam.jpg", le mandamos la foto
  server.begin(); // Arrancamos el servidor
}

void loop() {
  server.handleClient(); // El ESP32 se queda esperando todo el tiempo a que alguien pida una foto
}