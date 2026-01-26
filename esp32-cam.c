#include <WebServer.h> // Librería para convertir el ESP32 en un servidor que entrega páginas o imágenes
#include <WiFi.h>      // Librería para que el ESP32 pueda crear o conectarse a redes Wi-Fi
#include <esp32cam.h>  // Librería especializada para controlar el hardware de la cámara

// Definimos el nombre y la clave de la red Wi-Fi que va a crear la placa
const char* Nombre = "RED_ROBOT"; 
const char* Contrasena = "12345678";

// Creamos el objeto "server" en el puerto 80, que es la "puerta" estándar para navegar por internet
WebServer server(80);

// Configuramos la resolución: 320x240 es un tamaño equilibrado para que el video no tenga retraso (lag)
static auto res_fija = esp32cam::Resolution::find(320, 240);

// Esta función es la que "cocina" y envía la imagen cuando alguien la solicita
void enviarFoto() {
  auto frame = esp32cam::capture(); // Captura un instante (un cuadro) de lo que ve el lente
  
  if (frame == nullptr) { // Si por algún error la cámara no capturó nada (está vacía)
    server.send(503, "", ""); // Envía un error 503 (Servicio no disponible) al navegador
    return; // Sale de la función para no intentar enviar algo que no existe
  }
  
  // ENCABEZADOS: Son instrucciones para el navegador (Chrome/Edge/Celular)
  // "no-cache" significa: "No guardes esta foto, siempre pídeme una nueva"
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache"); // Instrucción antigua de seguridad para borrar rastro
  server.sendHeader("Expires", "-1");      // Dice que la imagen caduca inmediatamente
  server.sendHeader("Connection", "close"); // Cierra la conexión rápido para liberar la red

  server.setContentLength(frame->size()); // Le avisa al navegador cuántos bytes pesa la imagen
  server.send(200, "image/jpeg");        // Código 200 (OK) y avisa que lo que sigue es una foto JPEG
  
  WiFiClient cliente = server.client();   // Abre el "túnel" de comunicación con el dispositivo que pidió la foto
  frame->writeTo(cliente);                // Envía todos los datos de la imagen por ese túnel

  // LIMPIEZA: Muy importante para que la placa no se trabe
  frame.reset(); // Borra la foto de la memoria RAM interna después de enviarla
}

void setup() {
  Serial.begin(115200); // Abre la comunicación con la computadora para ver mensajes de error

  using namespace esp32cam; // Atajo para no escribir "esp32cam::" a cada rato
  Config cfg;               // Creamos una "ficha de configuración" para la cámara
  cfg.setPins(pins::AiThinker); // Le decimos que nuestra placa es el modelo AI-Thinker
  cfg.setResolution(res_fija);  // Aplicamos el tamaño de imagen 320x240 que definimos arriba
  cfg.setBufferCount(2);        // Prepara dos espacios de memoria para que el video sea fluido
  cfg.setJpeg(80);              // Calidad de la imagen (0 a 100). 80 es muy buena nitidez
  
  // Intentamos encender la cámara. Si falla, avisamos por el monitor serie de la PC
  if (!Camera.begin(cfg)) {
    Serial.println("Error al iniciar cámara");
  }

  WiFi.mode(WIFI_AP);              // Configuramos la placa como "Punto de Acceso" (Crea su propio Wi-Fi)
  WiFi.softAP(Nombre, Contrasena); // Iniciamos la red con el nombre y clave de arriba

  // Esta línea es clave: "Cuando alguien entre a la dirección /cam.jpg, ejecuta la función enviarFoto"
  server.on("/cam.jpg", enviarFoto); 
  
  server.begin(); // Encendemos oficialmente el servidor para empezar a recibir visitas
}

void loop() {
  server.handleClient(); // El ESP32 se queda revisando si alguien entró a la página
  
  delay(1); // Una pausa de 1 milisegundo para que el procesador no se sature y pueda manejar el Wi-Fi
}