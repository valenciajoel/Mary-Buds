#include <Arduino.h>
#include <FS.h>
#include <SPI.h>
#include <SD.h>
//#include <SdFat.h>
//#include <sqlite3.h>


//#include <Sqlite3Esp32.h>

const int chipSelect = 15; // Pin CS del módulo SD
const char* databaseFileName = "/maryBudsBD.db"; // Nombre del archivo de base de datos
const char* tableName = "productos"; // Nombre de la tabla de la base de datos

void setup() {
 
  Serial.begin(9600);
// Inicializar la tarjeta microSD
  if (!SD.begin(chipSelect)) {
    Serial.println("Error al inicializar la tarjeta microSD.");
    return;
  }
  Serial.println("Tarjeta microSD inicializada correctamente.");
/*
  // Abrir la base de datos SQLite
  sqlite3* db;

  if (sqlite3_open_v2("/sdcard/test.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
    Serial.println("Error al abrir la base de datos SQLite.");
    return;
  }

 Serial.println("Base de datos SQLite abierta correctamente.");

  // Crear una tabla en la base de datos SQLite
  if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, value INTEGER)", NULL, NULL, NULL) != SQLITE_OK) {
    Serial.println("Error al crear la tabla.");
    return;
  }
  Serial.println("Tabla creada correctamente.");

  // Insertar datos en la tabla
  if (sqlite3_exec(db, "INSERT INTO test_table (id, value) VALUES (1, 10)", NULL, NULL, NULL) != SQLITE_OK) {
    Serial.println("Error al insertar datos.");
    return;
  }
  Serial.println("Datos insertados correctamente.");

*/
}

void loop() {
  // Tu código aquí
}


