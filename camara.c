#include <WebServer.h>   // Crea el servidor
#include <WiFi.h>        // Permite controlar las funciones de red
#include <esp32cam.h>    // Para controlar la camara

// Estos serán ahora el nombre y la clave de la red que el ESP32 va a crear
const char* Nombre = "Rocks";       
const char* Contraseña = "sombras199";

const int pinFlash = 4;    // El LED 
const int resolucion = 8;  // Usamos 8 bits, lo que nos da un rango de 0 a 255 para el brillo

WebServer server(80); // Creamos el servidor en el puerto 80 (el estándar para internet)

// Configuramos que la imagen sea de 800x600 (según tu ajuste fijo)
static auto fixedRes = esp32cam::Resolution::find(800, 600);

// Esta función se encarga de sacar la foto y mandarla a tu computadora
void serveJpg() {
  auto frame = esp32cam::capture(); // Captura una foto en este instante
  if (frame == nullptr) {           // Si la cámara falla y no hay foto...
    Serial.println("Error al capturar imagen"); // Avisa por el monitor serial
    server.send(503, "", "");       // Envía un error al navegador
    return;
  }
  // Si la foto está bien, le decimos al navegador cuánto mide y qué tipo de imagen es
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client(); // Prepara el envío de datos
  frame->writeTo(client);               // Envía la foto bit por bit a la computadora
}

// Esta función recibe el dato "brillo" que mandas desde Python según tus dedos
void manejarControlFlash() {
  if (server.hasArg("brillo")) {          // Recibe el valor en texto
    String valorTexto = server.arg("brillo"); // Guarda el valor que llegó 
    int nivelBrillo = valorTexto.toInt(); // Convierte ese texto en un número real
    
    // El comando constrain asegura que el número nunca sea menor a 0 ni mayor a 255
    nivelBrillo = constrain(nivelBrillo, 0, 255);
    
    // ledcWrite aplica físicamente el voltaje al LED para que brille más o menos
    ledcWrite(pinFlash, nivelBrillo); 
    
    // Respondemos a Python que ya hicimos el cambio para que no se quede esperando
    server.send(200, "text/plain", "Nivel de brillo actualizado");
  }
}

void setup() {
  Serial.begin(115200); // Iniciamos la comunicación con la computadora
  Serial.println();

  //configuracion de la camara
  {
    using namespace esp32cam; 
    Config cfg; 
    cfg.setPins(pins::AiThinker); // Le dice a la placa que es el modelo AI-Thinker
    cfg.setResolution(fixedRes);  // Aplica el tamaño de imagen
    cfg.setBufferCount(2);        // Crea dos espacios de memoria para que el video sea fluido
    cfg.setJpeg(80);              // Calidad de la imagen (50 de 100)
    bool ok = Camera.begin(cfg);  // Intenta encender la cámara
    Serial.println(ok ? "Cámara lista" : "Error al iniciar cámara");
  }

  // Configuramos el pin del Flash con una frecuencia de 5000Hz (para que no parpadee)
  ledcAttach(pinFlash, 5000, resolucion);

  // crea su propia red wifi
  WiFi.mode(WIFI_AP); // Cambiamos a modo "Punto de Acceso" (la placa crea su red)
  
  // Creamos la red con el nombre y contraseña definidos arriba
  // Si todo sale bien, la placa empezará a emitir señal Wi-Fi
  WiFi.softAP(Nombre, Contraseña); 

  Serial.println("Iniciando punto de acceso...");
  Serial.print("Nombre de la red (SSID): ");
  Serial.println(Nombre);
  
  // Obtenemos la IP que el ESP32 se asignó a sí mismo como jefe de la red
  // Por defecto suele ser 192.168.4.1
  Serial.print("Dirección IP para conectar desde Python: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/cam.jpg", serveJpg);           // Si entras a /cam.jpg, ejecuta la función de la foto
  server.on("/control", manejarControlFlash); // Si entras a /control, ejecuta la función del brillo
  server.begin();                            // Enciende el servidor finalmente
}

void loop() {
  server.handleClient(); // Revisa todo el tiempo si Python le está mandando algo
  delay(100);
}