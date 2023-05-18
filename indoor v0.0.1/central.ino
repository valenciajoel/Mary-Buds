// LIBRERIAS
#include "sdGlb.h"
#include "connection.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>

// VARIABLES

// ESP
const int chipId = ESP.getChipId();

//GENERALES
String nombre = "central";
String ssidAPDefault = nombre   + chipId;


// rutas
String rutaCensado = "/historial/";
String rutaActuador = "/dispositivo/actuador/";
String rutaSensor = "/dispositivo/sensor/";
String rutaVinculo = "/vinculo/";
String rutaConfiguracion = "/configuracion/";
String rutaCredencialesWifi = rutaConfiguracion + "wifiCredenciales.txt";
String rutaCredencialesAP = rutaConfiguracion + "apCredenciales.txt";
String rutaContadorDispositivosVinculados = rutaConfiguracion + "contadorDispositivosVinculados.txt";



// wifi
String wifiSsid;
String wifiPassword;
int contWifi=0;
int intentosMaxWifi=0;
bool conectadoWifi=false;
int status=-2;

// access point
String apSsid;
String apPassword;

// puertos
unsigned int portUDP = 61570;
unsigned int portTCP = 65127;
unsigned int portServer = 80;

// Objetos
SdGlb sd(15);
Connection connection(portUDP, portTCP);
ESP8266WebServer server(portServer);

//UDP
String updMensaje="";

// Pin
const int CS_PIN = 15;

// pin conectado al CS del módulo de la tarjeta SD
//#define CS_PIN
// File archivo;			// objeto archivo del tipo File
// String mensajeUDP;

// unsigned long milisegundos = millis();

void setup()
{

  // Abrir puerto serial
  Serial.begin(9600);
  Serial.println("");
  Serial.print("ChipId:");
  Serial.println(String(chipId));

 // sd.eliminarCarpeta("/");


  //INCIALIZACION DE VARIABLES
  contWifi=0;


  if(!sd.existe(rutaCredencialesWifi)){
  Serial.println("No existen credenciales WIFI");
  connection.prenderAP(ssidAPDefault,"",false,8,20);
  }
  else{
  Serial.println("Obteniendo credenciales WIFI");

    // Leer las credenciales
    String auxWifi = sd.leerArchivo(rutaCredencialesWifi);

    // Parseo de texto
    wifiSsid = sd.parsearCSV(auxWifi, 1, ',');
    wifiPassword = sd.parsearCSV(auxWifi, 2, ',');

    // Inicio conexion WIFI
    do{
        conectadoWifi = connection.conectarWifi(wifiSsid, wifiPassword, 30);
        contWifi++;

    }while(!conectadoWifi && contWifi < intentosMaxWifi);

    
    if(!conectadoWifi){

        Serial.print("No se pudo conectar credenciales al WIFI");

        if (sd.existe(rutaCredencialesAP))
      {
        Serial.println("Obteniendo credenciales AP");
        // Leer las credenciales
        String auxAP = sd.leerArchivo(rutaCredencialesAP);
        // Parseo de texto
        apSsid = sd.parsearCSV(auxAP, 1, ',');
        apPassword = sd.parsearCSV(auxAP, 2, ',');
        connection.prenderAP(apSsid, apPassword, false, 8, 20);
      }
  }
}


  // Metodos expuestos del servidor
  server.on("/evento", procesarEvento);                    // procesa los vinculos y activa los actuadores correspondientes al evento
  server.on("/guardarVinculo", guardarVinculo);            // guarda el vinculo sensor-actuador
  server.on("/borrarVinculo", borrarVinculo);              // borra el vinculo sensor-actuador
  server.on("/guardarCrendenciales", guardarCredenciales); // guarda credenciales (WIFI)
  server.on("/credencialesWifi", credencialesWifi);        // envia las credenciales (WIFI) actuales de la central
  server.on("/sincronizado", descontarContador);           // resta al contador de dispositivos sincronizados
  server.on("/registrarSensor", guardarSensor);            // registra un sensor
  server.on("/registrarActuador", guardarActuador);        // registra un actuador
  server.on("/guardarCenso", guardarCensado); 
  server.on("/borrarActuador", borrarActuador);             //borra el registro del actuador
  server.on("/borrarSensor", borrarSensor);                 //borrar el archivo del sensor registrado y sus vinculos


  server.on("/validarCredencialesWifi", validarCredencialesWifi);       
    

  //actualizar IP tanto de sensor como de actuador


  //todo el mundo sabe que la jarra es con ferné

  // Inicio el server
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{

// inicio el server
  server.handleClient();



  updMensaje = connection.escucharUDP();



  //envio peticion get si el mensaje es para el chipset adecuado (el que poseo)

      //ESTRUCTURA DEL MENSAJE
      // TIPO MENSAJE/EVENTO , CHIPSET DEL RECEPTOR
      // 1,253651,192.168.0.255

  if(sd.parsearCSV(updMensaje,1,',').toInt() == 1 && sd.parsearCSV(updMensaje,2,',') == String(chipId)){
    connection.enviarIp(5);
  }

}

// descuenta el contador de dispositivos sincronizados
void descontarContador()
{

  //FALTA VERIFICAR SI ES UN DISPOSITIVO YA VINCULADO CON ANTERIORIDAD
  
  
  // si existe dispositivos sin vincular
  if (sd.existe(rutaContadorDispositivosVinculados))
  {

    // obtengo el valor del contador
    int cont = sd.leerArchivo(rutaContadorDispositivosVinculados).toInt();

    // elimino el archivo
    sd.eliminar(rutaContadorDispositivosVinculados);

    // resto en el contador
    cont--;

    // si es distinto de 0, guardo el archivo con el contador actualizado
    if (cont != 0)
    {
      sd.escribirArchivo(rutaContadorDispositivosVinculados, String(cont), false);
    }
  }
}

// envia las credenciales de WIFI
void credencialesWifi()
{
  server.send(200, "text/plain", wifiSsid + "," + wifiPassword);
}

//cuanta los dispositivos totales
int dispositivosTotales()
{

  // VARIABLES
  int total = 0;
  int actuadores = sd.contarArchivosTotales(rutaActuador);
  int sensores = sd.contarArchivosTotales(rutaSensor);

  // si existen actuadores
  if (actuadores != -1)
  {
    // acumulo
    total = total + actuadores;
  }
  // si existen sensores
  if (sensores != -1)
  {
    // acumulo
    total = total + sensores;
  }
  return total;
}

//registro credenciales WIFI
void guardarCredenciales()
{

  // Variables
  String wifiSsid = server.arg("wifiSsid");
  String wifiPassword = server.arg("wifiPassword");

    Serial.print("wifiSsid:");
    Serial.println(String (wifiSsid.length()));
    Serial.print("wifiPassword:");
    Serial.println(String(wifiPassword.length()));
    
  // Existe el archivo de las credenciales
  if (sd.existe(rutaCredencialesWifi))
  {
    // Lo elimino
    sd.eliminar(rutaCredencialesWifi);

    sd.eliminar(rutaContadorDispositivosVinculados);

    // creo el archivo con la cantidad de dispositivos que faltan vincular
    sd.escribirArchivo(rutaContadorDispositivosVinculados, String(dispositivosTotales()),false);
  }

  // Guardo las credenciales
  if (sd.escribirArchivo(rutaCredencialesWifi, wifiSsid + "," + wifiPassword,false))
  {
    // Envio mensaje exitoso
    server.send(201, "text/plain", "credenciales guardadas");
  }
  else
  {
    // Envio mensaje fallido
    server.send(400, "text/plain", "no se pudo guardar las credenciales");
  }

  // crear credenciales AP
    if(!sd.existe(rutaCredencialesAP)){
        Serial.println("crear archivo de credenciales AP");
        sd.escribirArchivo(rutaCredencialesAP,String(chipId) + "," + generarPassword(8),false);
    }


}


void borrarActuador(){

// VARIABLES
  String chipsetActuador = server.arg("chipsetActuador");
  String tipo = server.arg("tipo");
    // si existe el archivo de vinculo
  if (sd.existe(rutaActuador + "/" + tipo + "/" + chipsetActuador + ".txt"))
  {

    // si puedo eliminarlo
    if (sd.eliminar(rutaActuador + "/" + tipo + "/" + chipsetActuador + ".txt"))
    {
      
      // envio mensaje de exito
      server.send(200, "text/plain", "archivo borrado");
          Serial.println("Se elimino el archivo");
    }
    else
    {
      // envio mensaje de error al borrar
      server.send(400, "text/plain", "no se pudo borrar el archivo");
       Serial.println("No se pudo eliminar el archivo");
    }
  }
  else
  {

    // envio mensaje de archivo no encontrado
    server.send(204, "text/plain", "no existe el archivo");

    Serial.println("No existe el archivo");
  }


}


void borrarSensor(){

  // VARIABLES
  String chipsetSensor = server.arg("chipsetSensor");

   // si existe el archivo de vinculo
  if (sd.existe(rutaSensor + chipsetSensor +".txt"))
  {

    // si puedo eliminarlo
    if (sd.eliminar(rutaSensor + chipsetSensor +".txt"))
    {
      // envio mensaje de exito
      Serial.println("Archivo sensor borrado con exito");
    }
    else
    {
      // envio mensaje de error al borrar
      Serial.println("Archivo sensor no se pudo borrar");
    }

    
    if(sd.existe(rutaVinculo + chipsetSensor )){

      if(sd.eliminar(rutaVinculo + chipsetSensor)){

        Serial.println("se borro los vinculos con exito");


      }else{
        Serial.println("No se pudo borrar los vinculos del sensor");


      }

    }


  }
  else
  {

    // envio mensaje de archivo no encontrado
      Serial.println("No existe el archivo del sensor");
  }


}






//borrar un vinculo
void borrarVinculo()
{
  // VARIABLES
  String chipsetSensor = server.arg("chipsetSensor");

  // si existe el archivo de vinculo
  if (sd.existe(rutaVinculo + chipsetSensor + ".txt"))
  {

    // si puedo eliminarlo
    if (sd.eliminar(rutaVinculo + chipsetSensor + ".txt"))
    {

      // envio mensaje de exito
      server.send(200, "text/plain", "archivo borrado");
    }
    else
    {
      // envio mensaje de error al borrar
      server.send(400, "text/plain", "no se pudo borrar el archivo");
    }
  }
  else
  {

    // envio mensaje de archivo no encontrado
    server.send(204, "text/plain", "no existe el archivo");
  }
}

//guardo un vinculo
void guardarVinculo()
{

  //NUEVO


  // VARIABLES
  String chipsetActuador = server.arg("chipsetActuador");
  String tipoActuador = server.arg("tipo");
  String chipsetSensor = server.arg("chipsetSensor");
  String evento = server.arg("evento");

  // escribo un vinculo y lo guardo en un booleano
  bool guardado = sd.escribirArchivo(rutaVinculo + chipsetSensor + "/" + evento + "/" + chipsetActuador + ".txt", tipoActuador,false);
 // bool guardado = sd.escribirArchivo(rutaVinculo + chipsetSensor + ".txt", evento + "," + tipoActuador + "," + chipsetActuador + ";",true);

  if (guardado)
  {

    server.send(201, "text/plain", "vinculo guardado con exito");
  }
  else
  {
    server.send(404, "text/plain", "no se pudo guardar el vinculo");
  }
}

/*
//guardo un vinculo
void guardarVinculo()
{
  // VARIABLES
  String chipsetActuador = server.arg("chipsetActuador");
  String tipoActuador = server.arg("tipo");
  String chipsetSensor = server.arg("chipsetSensor");
  String evento = server.arg("evento");

  // escribo un vinculo y lo guardo en un booleano
  bool guardado = sd.escribirArchivo(rutaVinculo + chipsetSensor + ".txt", evento + "," + tipoActuador + "," + chipsetActuador + ";",true);

  if (guardado)
  {

    server.send(201, "text/plain", "vinculo guardado con exito");
  }
  else
  {
    server.send(404, "text/plain", "no se pudo guardar el vinculo");
  }
}
*/
//procesa un evento

void procesarEvento()
{

  //NUEVO

  // VARIABLES
  String evento = server.arg("evento");
  String ruta = rutaVinculo + server.arg("chipset") + "/" + evento;
  enviarMensajeVinculado(ruta,evento);

  server.send(200, "text/plain", "alerta recibida");
}

/*

void procesarEvento()
{
  // VARIABLES
  String message;
  String ruta = rutaVinculo + server.arg("chipset") + ".txt";
  int evento = server.arg("tipoAlerta").toInt();
  String mensaje = server.arg("mensaje");

  enviarMensajeVinculado(evento, ruta, mensaje);

  server.send(200, "text/plain", "alerta recibida");
}

*/
String generarPassword(int caracteresTotales){

char abecedario [] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','w','x','y','z'};
int cont=0;
String respuesta;

while(cont<caracteresTotales){
char c = (char) abecedario[random(0,34)];
respuesta += String(c);
cont++;
}
return respuesta;
}


void enviarMensajeVinculado(String ruta, String evento){

  //NUEVO

  // VARIABLES
   File root;
   File archivo;
   String tipo;


  if(sd.existe(ruta)){
    root = sd.abrirArchivo(ruta);
 do{
    archivo = (root.openNextFile());
    if(!archivo){
        //Si no hay archivo siguiente
        Serial.println("No hay mas archivos");
    }else{
      Serial.println(archivo.name());  //Imprimo el nombre
      tipo = sd.leerArchivo(ruta+"/"+archivo.name());

      if(enviarAlerta(archivo.name(),evento,tipo) == 2 ){
        
        Serial.println("No existe el dispositivo que se intento enviar alerta");

        if(sd.eliminar(ruta+"/"+archivo.name())){

            Serial.println("Eliminado con exito");

        }else{

            Serial.println("No se pudo eliminar");
        }




      };

    }
  }while(archivo);
archivo.close();
root.close();
  }else{

    server.send(404, "text/plain", "No existe la ruta del evento");
  }


}





int enviarAlerta(String chipsetActuador, String evento, String tipo)
{

  /*
  La respuesta puede ser
  0 = no se pudo enviar
  1 = enviado
  2 = no existe el archivo*/
  int respuesta;
  String ruta = rutaActuador + tipo + "/" + chipsetActuador + ".txt";

  //chequeo que el archivo exista
  if(sd.existe(ruta)){
    String ip = sd.leerArchivo(ruta);
    Serial.println("------------------------");
    Serial.print("Ruta:");
    Serial.println(ruta);
    Serial.print("Tipo:");
    Serial.println(String(tipo));
    Serial.print("IP:");
    Serial.println(ip);
/*
    //enviar el evento al actuador
    if (connection.enviarAvisoActuador(ip,evento,5) == 200)
    {
      respuesta = 1;
    }else{respuesta = 0;}
    */
  }else{
  
  respuesta = 2;}

return respuesta;
  
}


//guarda un censado
void guardarCensado()
{
  //VARIABLES
  String chipset = server.arg("chipset");
  String censo = server.arg("censo");

  // elimino el archivo
  sd.eliminar(rutaCensado + chipset + ".txt");

  if (sd.escribirArchivo(rutaCensado + chipset + ".txt", censo,false))
  {

    server.send(201, "text/plain", "censado guardado con exito");

  }else{

    server.send(400, "text/plain", "no se pudo guardar el censado");

  }

}

//registra un actuador
void guardarActuador()
{

  //VARIABLES
  String chipset = server.arg("chipset");
  String ip = ipToString(server.client().remoteIP());
  int tipo = server.arg("tipo").toInt();
  
  // elimino el archivo
  sd.eliminar(rutaActuador + tipo + "/" + chipset + ".txt");

  if (sd.escribirArchivo(rutaActuador + tipo + "/" + chipset + ".txt", ip,false))
  {
    server.send(201, "text/plain", "actuador registrado con exito");
  }else{
    server.send(400, "text/plain", "no se pudo registrar actuador");
  }

}

//registra un sensor
void guardarSensor()
{
  String chipset = server.arg("chipset");

  String ip = ipToString(server.client().remoteIP());

  // elimino el archivo
  sd.eliminar(rutaSensor + chipset + ".txt");

  if (sd.escribirArchivo(rutaSensor + chipset + ".txt", ip,false))
  {

    server.send(201, "text/plain", "sensor registrado con exito");
  }else{

    server.send(400, "text/plain", "no se pudo registrar sensor");
  }

}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++){
    s += i  ? "." + String(ip[i]) : String(ip[i]);}
  return s;
}

void validarCredencialesWifi()
{
String ssid = server.arg("ssid");
String password = server.arg("password");


if(status == -2 ){
server.send(200, "text/plain", "credenciales recibidas");

delay(4000);
status = connection.conectarWifi(ssid,password,30);
connection.prenderAP(ssidAPDefault,"",false,8,20);
//status = connection.validarCredencialesWifi(ssid,password,30);
}

else{

 if (status == WL_CONNECTED)
  {
    Serial.println("conexion exitosa");
    server.send(200, "text/plain", "credenciales validas");
  }
  else{

    if(status == WL_CONNECT_FAILED){

    Serial.println("contraseña incorrecta");
    server.send(200, "text/plain", "contraseña incorrecta");

    }else{

    if(status == WL_NO_SSID_AVAIL){

    Serial.println("no se encuentra la red que se quiere configurar");
    server.send(200, "text/plain", "no se encuentra la red que se quiere configurar");

      
    }else{
    server.send(200, "text/plain", "no se encuentra la red que se quiere configurar");

    }

  }


}



}






}
