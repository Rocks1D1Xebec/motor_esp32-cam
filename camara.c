//ESTE CODIGO LO DEBEN PEGAR EN EL ARDUINO IDE
#include <WebServer.h>//server
#include <WiFi.h>//conectarse a wifi
#include <esp32cam.h>//control de camara

const char* Nombre = "Nyls";
const char* Contraseña = "sombras199";

//---configuracion del motor---
const int pinMotor = 12;      //pin para el motor
const int frecuencia = 5000;  //velocidad de la señal
const int resolucion = 8;     //resolucion de 0 a 255 (8 bits)

WebServer server(80);

static auto fixedRes = esp32cam::Resolution::find(320, 240); //dimensiones de la imagen

void serveJpg() {
  auto frame = esp32cam::capture();//captura imgen
  if (frame == nullptr) {
    Serial.println("Error al capturar img");
    server.send(503, "", "");
    return;
  }
  server.setContentLength(frame->size());//cuanto pesa la imagen
  server.send(200, "image/jpeg");//indica que es una imgen jpeg
  WiFiClient client = server.client();//identifica al cliente 
  frame->writeTo(client);//envia la img
}

void manejarControl() {
  if (server.hasArg("potencia")) { //revisa si llego el dato potencia
    String valorTexto = server.arg("potencia");//guarda el texto recibido
    int valorPotencia = valorTexto.toInt();//lo convierte a numero
    valorPotencia = constrain(valorPotencia, 0, 255);//asegura rango de 0 a 255
    //Ahora escribimos directamente al PIN, no al canal
    ledcWrite(pinMotor, valorPotencia); //manda la potencia al pin 12
    
    server.send(200, "text/plain", "ok");//responde al python
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  {
    using namespace esp32cam; 
    Config cfg;               
    cfg.setPins(pins::AiThinker);//configura los pines automatico
    cfg.setResolution(fixedRes);//aplica el tamaño de la imagen
    cfg.setBufferCount(2);//2 espacios para captura
    cfg.setJpeg(80); // Calidad de imagen
    bool ok = Camera.begin(cfg);//inicia la camara
    Serial.println(ok ? "Inicio exitoso" : "Inicio fallido");
  }

  //---NUEVA FORMA DE CONFIGURAR EL MOTOR (ESP32 V3.0)---
  // Esta sola linea reemplaza a ledcSetup y ledcAttachPin
  ledcAttach(pinMotor, frecuencia, resolucion); //conecta el pin con su config

  WiFi.persistent(false);//mantiene los datos solo en ram
  WiFi.mode(WIFI_STA);//conectarse al wifi
  WiFi.begin(Nombre, Contraseña);//usa los datos de red
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Conectado");
  Serial.print("URL camara: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/cam.jpg");
  Serial.print("URL Motor: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/control"); // Link para probar el motor
  
  server.on("/cam.jpg", serveJpg);//ruta para la imagen
  server.on("/control", manejarControl);//ruta para motor
  server.begin();//inicia el server
}

void loop() {
  server.handleClient();//procesa peticiones
}