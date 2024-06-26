#include <WiFi.h>
#include <ArduinoJson.h>
#include "DHT.h"

#include <secrets.h>

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

#define PIN_PC_ON 6 //Pin para la señal de encendido

#define DHT_PIN 17     // Pin GPIO al que está conectado el sensor DHT22
#define DHT_TYPE DHT22 // Tipo de sensor DHT

DHT dht(DHT_PIN, DHT_TYPE);

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
                "<form action='/control' method='GET'>"
                "<input name='secret_class'>"
                "<button type='submit' name='on' value='ON' style='height:50px;width:100px'>ON</button>"
                "<button type='submit' name='off' value='OFF' style='height:50px;width:100px'>OFF</button>"
                "</form>"
                "</center>"
                "</body>"
                "</html>";


//---------------------------SETUP--------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("");

  pinMode(PIN_PC_ON, OUTPUT);

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
  digitalWrite(PIN_PC_ON, HIGH); // Enciende el pin
  delay(500); // Espera 0.5 segundos
  digitalWrite(PIN_PC_ON, LOW); // Apaga el pin

  //depuración
  Serial.println("PC on");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
}

bool getTemperatureAndHumidity(float &temperature, float &humidity) {
  dht.begin();
  // Intenta leer datos del sensor
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Verifica si la lectura fue exitosa
  if (isnan(temp) || isnan(hum)) {
    return false; // Error al leer datos del sensor, no devuelve nada
  } else {
    temperature = temp;
    humidity = hum;
    return true; // Lee datos correctamente y los devuelve
  }
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

            // enciende y apaga el GPIO
            if (header.indexOf("GET /control") >= 0) {
              if (header.indexOf("GET /control?secret_class="+ String(PC_ON_KEY) +"&on=ON") >= 0) {
                PC_ON();
              } else {
                // If the secret class doesn't match, send an error response
                client.println("HTTP/1.1 403 Forbidden");
                client.println("Content-type:text/html");
                client.println("Connection: close");
                client.println();
                client.println("<h1>Forbidden</h1><p>You don't have permission to access this resource.</p>");
              }
            } else if (header.indexOf("GET /getTempAndHumd") >= 0) {
              // Iniciar la comunicación con el sensor DHT y obtener la temperatura y humedad
              float temperature, humidity;
              bool success = getTemperatureAndHumidity(temperature, humidity);

              Serial.print(temperature);
              Serial.print(humidity);

              // Crear un objeto JSON para almacenar los datos de temperatura y humedad
              DynamicJsonDocument doc(200);
              doc["temperature"] = success ? temperature : NAN; // Si la lectura falla, establece NaN (Not a Number)
              doc["humidity"] = success ? humidity : NAN;

              // Convertir el objeto JSON en una cadena
              String response;
              serializeJson(doc, response);

              // Enviar la respuesta al cliente
              client.print("HTTP/1.1 200 OK\r\n");
              client.print("Content-Type: application/json\r\n");
              client.print("Connection: close\r\n");  // Cierra la conexión después de enviar la respuesta
              client.println("Content-Length: " + String(response.length()));
              client.println();
              client.println(response);

              // Esperar un momento para que el cliente reciba los datos
              delay(10);

              // Cerrar la conexión con el cliente
              client.stop();
            } else { // Default
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");                                                
              client.println("Connection: close");
              client.println();
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