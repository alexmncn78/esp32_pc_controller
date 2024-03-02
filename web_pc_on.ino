#include <WiFi.h>

//------------------Servidor Web en puerto X ---------------------
short puerto = 80;
WiFiServer server(puerto);

//---------------------Credenciales de WiFi-----------------------

//Wifi piso
const char* ssid_piso     = "MiFibra-0A20-24G";
const char* password_piso = "9XKrvXK2";

//Wifi casa
const char* ssid_casa     = "LowiC73C";
const char* password_casa = "DH63H4KU676JPC";

//---------------------VARIABLES GLOBALES-------------------------
short contconexion = 0;

String header; // Variable para guardar el HTTP request

#define PIN_ON 6 //Pin para la señal de encendido

//------------------------CODIGO HTML------------------------------
String pagina = "<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='utf-8' />"
"<title>Servidor Web ESP32</title>"
"</head>"
"<body>"
"<center>"
"<h1>Servidor Web ESP32</h1>"
"<p><a href='/on'><button style='height:50px;width:100px'>ON</button></a></p>"
"<p><a href='/off'><button style='height:50px;width:100px'>OFF</button></a></p>"
"</center>"
"</body>"
"</html>";


//---------------------------SETUP--------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("");

  pinMode(PIN_ON, OUTPUT);

  // Conexión WIFI
  short estado;
  short estado_piso = 1;
  short estado_casa = 2;

  estado = 1;
  while(estado == estado_piso || estado == estado_casa){
    if (estado == estado_piso) {
      IPAddress ip(192,168,1,100); 
      IPAddress gateway(192,168,1,1); 
      IPAddress subnet(255,255,255,0); 
      WiFi.config(ip, gateway, subnet);

      WiFi.begin(ssid_piso, password_piso);
      Serial.println("Intentando conexión con wifi 'piso'");
    }else if(estado == estado_casa){
      IPAddress ip(192,168,0,100); 
      IPAddress gateway(192,168,0,1); 
      IPAddress subnet(255,255,255,0); 
      WiFi.config(ip, gateway, subnet);

      //reseteamos el contador
      contconexion = 0;

      WiFi.begin(ssid_casa, password_casa);
      Serial.println("Intentando conexión con wifi 'casa'");
    }

    //Cuenta hasta 20 si no se puede conectar lo cancela
    while (WiFi.status() != WL_CONNECTED and contconexion <10) { 
      ++contconexion;
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      Serial.print(".");
      digitalWrite(LED_BUILTIN, HIGH);
    }

    //Comprobamos el estado de la conexión
    if (WiFi.status() == WL_CONNECTED) { 
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.println(WiFi.localIP());
      server.begin(); // iniciamos el servidor
      //Esperamos 3 seg para indicar con el LED que estamos conectados
      delay(3000);
      digitalWrite(LED_BUILTIN, LOW);
      estado=0;
    }else { 
      Serial.println("");
      Serial.println("Error de conexion\n");
      estado = estado + 1;
    }
  }

}

void PC_ON(){
  //Mandamos señal para encender
  digitalWrite(PIN_ON, HIGH); // Enciende el pin
  delay(500); // Espera 0.5 segundos
  digitalWrite(PIN_ON, LOW); // Apaga el pin

  //depuración
  Serial.println("PC on");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
}

//----------------------------LOOP----------------------------------

void loop(){
  WiFiClient client = server.available();   // Escucha a los clientes entrantes

  if (client) {                             // Si se conecta un nuevo cliente
    Serial.println("New Client.");          // 
    String currentLine = "";                //
    while (client.connected()) {            // loop mientras el cliente está conectado
      if (client.available()) {             // si hay bytes para leer desde el cliente
        char c = client.read();             // lee un byte
        Serial.write(c);                    // imprime ese byte en el monitor serial
        header += c;
        if (c == '\n') {                    // si el byte es un caracter de salto de linea
          // si la nueva linea está en blanco significa que es el fin del 
          // HTTP request del cliente, entonces respondemos:
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");                                                
            client.println("Connection: close");
            client.println();
            
            // enciende y apaga el GPIO
            if (header.indexOf("GET /on") >= 0) {
              PC_ON();

            } else if (header.indexOf("GET /off") >= 0) {
                //nada 
            }
              
            // Muestra la página web
            client.println(pagina);

            // la respuesta HTTP temina con una linea en blanco
            client.println();
            break;

          } else { // si tenemos una nueva linea limpiamos currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // si C es distinto al caracter de retorno de carro
          currentLine += c;      // lo agrega al final de currentLine
        }
      }
    }
    // Limpiamos la variable header
    header = "";
    // Cerramos la conexión
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}