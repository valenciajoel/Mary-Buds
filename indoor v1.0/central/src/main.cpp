#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);
const char* miRed = "ESP8266";
const char* miContrasena = "hola";
const uint32_t TiempoEsperaWifi = 5000;
void mensajeBase(){
  server.send(200, "text/plain","Hola desde el ESP");
}

void mensajeHumedad(){
String mensaje = "Valores recibidos del sensor:\n\n";
mensaje += "URI";
mensaje += server.uri();
mensaje += "\nMetodo: ";
mensaje += (server.method() == HTTP_GET) ? "GET" : "POST";
mensaje += "\nArgumentos: ";
mensaje += server.args();
mensaje += "\n";

for(int i = 0; i < server.args(); i++){
  mensaje += " " + server.argName(i) + ": " + server.arg(i) + "\n";
}

mensaje += "\n";

if(server.hasArg("sensor")){
  mensaje += "El sensor: " + server.arg("sensor");
  mensaje += "\nLa humedad es de: " + server.arg("humedad") + "%";
}else{
  mensaje += "No hay censado de ESP8266";
}
Serial.println(mensaje);
server.send(200, "text/plain", mensaje);
}

void setup() {
  Serial.begin(9600);
  const char * ssid = "Telecentro-bed";
  const char * password = "UY7NC49H9HRG";
  WiFi.begin(ssid, password); 
  int intento = 0;
  while (WiFi.status() != WL_CONNECTED && intento > 5) { // Espera a que se establezca la conexión
    delay(1000);
    Serial.print(".");
    Serial.println(intento);
    intento++;
  }
  if(intento == 5){
    WiFi.mode(WIFI_AP);
    WiFi.softAP(miRed,miContrasena);
  }
  Serial.println("");
  Serial.println("Conexión establecida");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  if(!MDNS.begin("central")){
    Serial.println("Error configurando mDNS!");
    while(1){
      delay(1000);
    }
  }

  MDNS.addService("http","tcp",80);

  server.on("/", mensajeBase);
  server.on("/humedad", mensajeHumedad);
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();
  MDNS.update();
}
