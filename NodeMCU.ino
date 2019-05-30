/*
 * Instituto Tecnologico de Costa Rica
 * Computer Engineering
 * Taller de Programacion
 * 
 * Código Servidor
 * Implementación del servidor NodeMCU
 * Proyecto 2, semestre 1
 * 2019
 * 
 * Profesor: Pedro Gutierrez
 * Autor: -Adrian -Julio 
 * 
 * Restricciónes: Biblioteca ESP8266WiFi instalada
 */
#include <ESP8266WiFi.h>


//Cantidad maxima de clientes es 1
#define MAX_SRV_CLIENTS 1
//Puerto por el que escucha el servidor
#define PORT 7070

/*
 * ssid: Nombre de la Red a la que se va a conectar el Arduino
 * password: Contraseña de la red
 * 
 * Este servidor no funciona correctamente en las redes del TEC,
 * se recomienda crear un hotspot con el celular
 */
const char* ssid = "Aparta 4";
const char* password = "jjhd0921";


// servidor con el puerto y variable con la maxima cantidad de 

WiFiServer server(PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];

/*
 * Intervalo de tiempo que se espera para comprobar que haya un nuevo mensaje
 */
unsigned long previousMillis = 0, temp = 0;
const long interval = 100;

/*
 * Pin donde está conectado el sensor de luz
 * Señal digital, lee 1 si hay luz y 0 si no hay.
 */


#define ldr D7

/**
 * Variables para manejar las luces con el registro de corrimiento.
 * Utilizan una función propia de Arduino llamada shiftOut.
 * shiftOut(ab,clk,LSBFIRST,data), la función recibe 2 pines, el orden de los bits 
 * y un dato de 8 bits.
 * El registro de corrimiento tiene 8 salidas, desde QA a QH. Nosotros usamos 6 de las 8 salidas
 * Ejemplos al enviar data: 
 * data = B00000000 -> todas encendidas
 * data = B11111111 -> todas apagadas
 * data = B00001111 -> depende de LSBFIRST o MSBFIRST la mitad encendida y la otra mitad apagada
 */
/**
#define OUTPUT4 D4
#define OUTPUT3 D3 // D4 en HIGH : retroceder
#define OUTPUT2 D2 // D3 en HIGH : avanzar
#define OUTPUT1 D1 // 
*/
int data = B00000000

//MOTOR DERECHA
int OUTPUT4 = 16;
int OUTPUT3 = 5;
//MOTOR IZQUIERDA
int OUTPUT2 = 4;
int OUTPUT1 = 0;



#define ab  D6 
#define clk D8






/**
 * Variables
 */
 
// #AGREGAR VARIABLES NECESARIAS 

int ENB = 2; /* GPIO02(D4) ->Motor-A Enable */
int ENA = 14; /* GPIO14(D5) ->Motor-B Enable */

/**
 * Función de configuración.
 * Se ejecuta la primera vez que el módulo se enciende.
 * Si no puede conectarse a la red especificada entra en un ciclo infinito 
 * hasta ser reestablecido y volver a llamar a la función de setup.
 * La velocidad de comunicación serial es de 115200 baudios, tenga presente
 * el valor para el monitor serial.
 */
void setup() {
  Serial.begin(115200);
  pinMode(OUTPUT4,OUTPUT);
  pinMode(OUTPUT3,OUTPUT);
  pinMode(OUTPUT2,OUTPUT);
  pinMode(OUTPUT1,OUTPUT);

  pinMode(ENB, OUTPUT); 
  pinMode(ENA, OUTPUT);

  pinMode(clk,OUTPUT);
  pinMode(ab,OUTPUT);
  
  pinMode(ldr,INPUT);

  // ip estática para el servidor
  IPAddress ip(192,168,0,200);
  IPAddress gateway(192,168,0,1);
  IPAddress subnet(255,255,255,0);

  WiFi.config(ip, gateway, subnet);

  // Modo para conectarse a la red
  WiFi.mode(WIFI_STA);
  // Intenta conectar a la red
  WiFi.begin(ssid, password);
  
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if (i == 21) {
    Serial.print("\nCould not connect to: "); Serial.println(ssid);
    while (1) delay(500);
  } else {
    Serial.print("\nConnection Succeeded to: "); Serial.println(ssid);
    Serial.println(".....\nWaiting for a client at");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Port: ");
    Serial.print(PORT);
  }
  server.begin();
  server.setNoDelay(true);




}

/*
 * Función principal que llama a las otras funciones y recibe los mensajes del cliente
 * Esta función comprueba que haya un nuevo mensaje y llama a la función de procesar
 * para interpretar el mensaje recibido.
 */
void loop() {
  
  unsigned long currentMillis = millis();
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      // El cliente existe y está conectado
      if (serverClients[i] && serverClients[i].connected()) {
        // El cliente tiene un nuevo mensaje
        if(serverClients[i].available()){
          // Leemos el cliente hasta el caracter '\r'
          String mensaje = serverClients[i].readStringUntil('\r');
          // Eliminamos el mensaje leído.
          serverClients[i].flush();
          
          // Preparamos la respuesta para el cliente
          String respuesta; 
          procesar(mensaje, &respuesta);
          Serial.println(mensaje);
          // Escribimos la respuesta al cliente.
          serverClients[i].println(respuesta);
        }  
        serverClients[i].stop();
      }
    }
  }
}

/*
 * Función para dividir los comandos en pares llave, valor
 * para ser interpretados y ejecutados por el Carro
 * Un mensaje puede tener una lista de comandos separados por ;
 * Se analiza cada comando por separado.
 * Esta función es semejante a string.split(char) de python
 * 
 */
void procesar(String input, String * output){
  //Buscamos el delimitador ;
  Serial.println("Checking input....... ");
  int comienzo = 0, delComa, del2puntos;
  bool result = false;
  delComa = input.indexOf(';',comienzo);
  
  while(delComa>0){
    String comando = input.substring(comienzo, delComa);
    Serial.print("Processing comando: ");
    Serial.println(comando);
    del2puntos = comando.indexOf(':');
    /*
    * Si el comando tiene ':', es decir tiene un valor
    * se llama a la función exe 
    */
    if(del2puntos>0){
        String llave = comando.substring(0,del2puntos);
        String valor = comando.substring(del2puntos+1);

        Serial.print("(llave, valor) = ");
        Serial.print(llave);
        Serial.println(valor);
        //Una vez separado en llave valor 
        *output = implementar(llave,valor); 
    }
    else if(comando == "sense"){
      *output = getSense();         
    }
    /**
     * ## AGREGAR COMPARACIONES PARA COMANDOS SIN VALOR
     * 
     */
    else if (comando == "CIRCLE") {
      digitalWrite(ENB,HIGH);
      digitalWrite(ENA,HIGH);
      digitalWrite(OUTPUT1, 1);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 500);
      delay(20000);
      digitalWrite(ENB,LOW);
      digitalWrite(ENA,LOW);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 0);
    }
    else if (comando == "INFINITE") {
      digitalWrite(ENB,HIGH);
      digitalWrite(ENA,HIGH);
      digitalWrite(OUTPUT1, 1);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 700);
      delay(10000);
      digitalWrite(ENB,HIGH);
      digitalWrite(ENA,HIGH);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 1);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 700);
      delay(10000);
      digitalWrite(ENB,LOW);
      digitalWrite(ENA,LOW);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 0);
    }
    else if (comando == "ZIGZAG") {
      digitalWrite(ENB,HIGH);
      digitalWrite(ENA,HIGH);
      digitalWrite(OUTPUT1, 1);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 1);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(OUTPUT1, 1);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 1);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(OUTPUT1, 1);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 1);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(OUTPUT1, 1);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 1);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 750);
      delay(500);
      digitalWrite(ENB,LOW);
      digitalWrite(ENA,LOW);
      digitalWrite(OUTPUT1, 0);
      digitalWrite(OUTPUT2, 0);
      analogWrite(OUTPUT3, 0);
      analogWrite(OUTPUT4, 0);
    }
    
    else{
      Serial.print("Comando no reconocido. Solo presenta llave");
      *output = "Undefined key value: " + comando+";";
    }
    comienzo = delComa+1;   
    delComa = input.indexOf(';',comienzo);
  }
}

String implementar(String llave, String valor){
  /**
   * La variable result puede cambiar para beneficio del desarrollador
   * Si desea obtener más información al ejecutar un comando.
   */
  String result="ok;";
  Serial.print("Comparing llave: ");
  Serial.println(llave);
  Serial.println("Valor: ");
  Serial.println(valor.toInt());
  Serial.println(valor);
  
  
  
  if(llave == "pwm"){
    Serial.print("Move....: ");
    Serial.println(valor);
    //# AGREGAR PARA CÓDIGO PARA MOVER EL CARRO HACIA DELANTE Y ATRAS
    switch (valor.toInt()){
      case -1023:  
        digitalWrite(ENB,HIGH);
        analogWrite(OUTPUT3, 1023);
        analogWrite(OUTPUT4, 0);
        break;      
      case 1023:
        digitalWrite(ENB,HIGH);
        analogWrite(OUTPUT3, 0);
        analogWrite(OUTPUT4, 1023);
        break; 
      default:
        digitalWrite(ENB,LOW);
        digitalWrite(OUTPUT3, 0);
        digitalWrite(OUTPUT4, 0);
        break;
      
    }
    
  }
  else if(llave == "dir"){
    switch (valor.toInt()){
      case 1:
        Serial.println("Girando derecha");
        digitalWrite(ENA,HIGH);
        digitalWrite(OUTPUT1, 1);
        digitalWrite(OUTPUT2, 0);
        break;
      case -1:
        Serial.println("Girando izquierda");
        digitalWrite(ENA,HIGH);
        digitalWrite(OUTPUT1, 0);
        digitalWrite(OUTPUT2, 1);
        break;
       default:
        Serial.println("directo");
        //# AGREGAR CÓDIGO PARA NO GIRAR
        digitalWrite(ENA,LOW);
        digitalWrite(OUTPUT1, 0);
        digitalWrite(OUTPUT2, 0); 
        break;
    }
  }
  else if(llave[0] == 'l'){
    Serial.println("Cambiando Luces");
    Serial.print("valor luz: ");
    Serial.println(valor);
    //Recomendación utilizar operadores lógico de bit a bit (bitwise operators)
    switch (llave[1]){
      case 'f':
        Serial.println("Luces frontales");
        //# AGREGAR CÓDIGO PARA ENCENDER LUCES FRONTALES
          data = data&front;
          break;
      case 'b':
        Serial.println("Luces traseras");
        //# AGREGAR CÓDIGO PARA ENCENDER O APAGAR LUCES TRASERAS
         data=B00000000;
         break;
         
      case 'l':
        Serial.println("Luces izquierda");
        //# AGREGAR CÓDIGO PARA ENCENDER O APAGAR DIRECCIONAL IZQUIERDA
        data=B11111111;
        shiftOut(ab, clk, LSBFIRST, data);
        break;



      case 'r':
        Serial.println("Luces derechas");
        //# AGREGAR PARA CÓDIGO PARA ENCENDER O APAGAR DIRECCIONAL DERECHA
        data = B10101011;
        digitalWrite(ab, HIGH);
        digitalWrite(clk, LOW);
        shiftOut(ab, clk, LSBFIRST, data);
        break;
      case 'a':
        Serial.println("Ninguna de las anteriores");
        data = B10000000;
        digitalWrite(ab, HIGH);
        digitalWrite(clk, HIGH);
        shiftOut(ab, clk, LSBFIRST, data);
        break;
      /**
       * # AGREGAR CASOS CON EL FORMATO l[caracter]:valor;
       * SI SE DESEAN manejar otras salidas del registro de corrimiento
       */
      default:
        Serial.println("Ninguna de las anteriores");
        data = B11111111;
        digitalWrite(ab, LOW);
        shiftOut(ab, clk, LSBFIRST, data);
        digitalWrite(ab, HIGH); 
        break;
    }
    //data VARIABLE QUE DEFINE CUALES LUCES SE ENCIENDEN Y CUALES SE APAGAN
    /*shiftOut(ab, clk, LSBFIRST, data);*/
  }
  /**
   * El comando tiene el formato correcto pero no tiene sentido para el servidor
   */
  else{
    result = "Undefined key value: " + llave+";";
    Serial.println(result);
  }
  return result;
}

/**
 * Función para obtener los valores de telemetría del auto
 */
String getSense(){
  //# EDITAR CÓDIGO PARA LEER LOS VALORES DESEADOS
  int batteryLvl = -1;
  int light = -1;

  // EQUIVALENTE A UTILIZAR STR.FORMAT EN PYTHON, %d -> valor decimal
  char sense [16];
  sprintf(sense, "blvl:%d;ldr:%d;", batteryLvl, light);
  Serial.print("Sensing: ");
  Serial.println(sense);
  return sense;
}
