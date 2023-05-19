#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
//#define WIFI_SSID "WIFI"
//#define WIFI_PASSWORD "tonelero69421"
#define WIFI_SSID "Telecentro-beed"
#define WIFI_PASSWORD "UY7NC49H9HRG"
/* 2. Define the RTDB URL */
#define DATABASE_URL "mary-buds-ddbb8-default-rtdb.firebaseio.com"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 3. Define the Firebase Data object */
FirebaseData fbdo;

/* 4, Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* Define the FirebaseConfig data for config data */
FirebaseConfig config;

unsigned long dataMillis = 0;

void setup() {

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the certificate file (optional) */
  // config.cert.file = "/cert.cer";
  // config.cert.file_storage = StorageType::FLASH;

  /* Assign the database URL(required) */
  config.database_url = DATABASE_URL;

  config.signer.test_mode = true;

  Firebase.reconnectWiFi(true);

  /* Initialize the library with the Firebase authen and config */
  Firebase.begin(&config, &auth);
}

void loop() {
  if (millis() - dataMillis > 5000) {
    dataMillis = millis();
  /*  
    String datoBdd = Firebase.getString(fbdo, "/test/int");
    char datoActual[10];  // Tamaño suficiente para almacenar la cadena de caracteres

    snprintf(datoActual, sizeof(datoActual), "%s", datoBdd.c_str());
    
    Serial.println(datoActual);
*/
    Serial.printf("Get string... %s\n", Firebase.getString(fbdo, F("/test/int")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
    /*
        //Serial.printf("Set int... %s\n", Firebase.setInt(fbdo, "/test/int", count++) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Ruta... %s\n", Firebase.setInt(fbdo, "/glb777/indoor/sensores/temperatura/ESP-6536/valor", 35) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Ruta... %s\n", Firebase.setInt(fbdo, "/glb777/indoor/sensores/humedad/ESP-6542/valor", 44) ? "ok" : fbdo.errorReason().c_str());
                Serial.printf("Ruta... %s\n", Firebase.setInt(fbdo, "/glb777/indoor/sensores/temperatura/ESP-6536/hisotorial/"+ (String) millis() +"/valor", 35) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Ruta... %s\n", Firebase.setString(fbdo, "/glb777/indoor/actuadores/ventilador/ESP-6825/idSensor", "ESP-6536") ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Ruta... %s\n", Firebase.setInt(fbdo, "/glb777/indoor/actuadores/ventilador/ESP-6825/minimo", 20) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Ruta... %s\n", Firebase.setInt(fbdo, "/glb777/indoor/actuadores/ventilador/ESP-6825/maximo", 30) ? "ok" : fbdo.errorReason().c_str());*/
  }
}

