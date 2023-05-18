#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Telecentro-beed";
const char* password = "UY7NC49H9HRG";
int sensorPin = A0;
WiFiClient cliente;
int humedad(){
  int sensorValue = analogRead(sensorPin); // Lee el valor analógico del pin D0
  float humidity = map(sensorValue, 1023, 577, 0, 100); // Convierte el valor analógico en un porcentaje de humedad 
  return humidity;
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a la red WiFi.");
}

void loop() {
  HTTPClient http;
  String sensor = "sensorHumedad";
  String url = "http://192.168.0.25/humedad?sensor="+ sensor + "&humedad=" + humedad();
  http.begin(cliente, url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }
  http.end();
  delay(5000); // Esperar 1 minuto antes de enviar otra petición GET
}