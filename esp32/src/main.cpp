#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"

//configuracao dos pinos para o modulo HX711
const int PINO_DT = 18;
const int PINO_SCK = 17;

const int TEMPO_ESPERA = 1000; //declaracao da variavel de espera

HX711 escala; //declaracao do objeto escala na classe HX711 da biblioteca

float fator_calibracao = 220000; //pre-definicao da variavel de calibracao

float kg = 0;

char comando; //declaracao da variavel que ira receber os comandos para alterar o fator de calibracao


#define ESP32_MODE_AP   1
#define ESP32_MODE_STA  2

#define ESP32_STATE_OFF 0
#define ESP32_STATE_STA 1
#define ESP32_STATE_AP  2

#define MAX_LEN         50

TaskHandle_t mqttConn; 
TaskHandle_t handleStartMQTT; 

byte esp32_wifi_mode      = ESP32_STATE_STA;
const char* ssid          = "Suzana";
const char* password      = "20461203";
long lastReconnectAttempt = 0;
bool mqttAlreadyRunning   = false;
float offset              = 0; //variável para guardar o valor bruto de offset
char buf[7]               = {0};
bool sendMsg              = false;

//HX711 scale;

struct configStruct{
  char ssid[MAX_LEN]         = {0};
  char senha[MAX_LEN]        = {0};
  char senha1[MAX_LEN]       = {0};
  char mqtt_host[MAX_LEN]    = {0};
  char mqtt_port[MAX_LEN]    = {0};
  char clientId[MAX_LEN]     = {0};
  char mqtt_pass[MAX_LEN]    = {0};
  char publish_to[MAX_LEN]   = {0};
  char subscribe_to[MAX_LEN] = {0};

  IPAddress ip;
  IPAddress gateway;
} wifiConfig;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void preLoad();
void enableInterrupt();
void IRAM_ATTR my_isr_handler(void* arg);
void startWifiMode(byte choice);
void vMQTT(void *pvParameters);
bool reconnect();
void callback(char* topic, byte* payload, unsigned int length);

void startWifiMode(byte choice){
  Serial.println(":: WiFi configuration ::");
  Serial.println("Stopping, if any...");

  if (choice == ESP32_MODE_STA){
      Serial.println("Starting station mode...");
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      esp32_wifi_mode = ESP32_STATE_STA;
      Serial.println(WiFi.localIP());
      Serial.println(":: WiFi Configuration - done ::\n\n");
      return;
  }
}

bool reconnect() {
  if (client.connect(wifiConfig.clientId,wifiConfig.clientId,wifiConfig.mqtt_pass)) {
    client.subscribe("test/topic");
  }
  return client.connected();
}

//TODO: tratar o payload?
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println( (char *) payload);
}

void vMQTT(void *pvParameters){
  vTaskDelay(pdMS_TO_TICKS(3000));
  if (WiFi.status() != WL_CONNECTED){
    Serial.print("WiFi not connected. Waiting for...");
    while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
 
  Serial.println("fazendo a conexao com o broker...");
  IPAddress srv(192,168,0,21);
  client.setServer("vemcompai.zapto.org", 1883);
  client.subscribe("test/topic");
  client.setCallback(callback);
  client.connect("balanca","","");
 
  mqttAlreadyRunning = true;
 
  while (true){
    client.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
    if (sendMsg){
      Serial.println("mandou enviar");
      client.publish("test/topic",buf);
      sendMsg = false;
    }
    
  }
}

void preLoad(){
  strcpy(wifiConfig.ssid,"Suzana");
  strcpy(wifiConfig.senha,"20461203");
  strcpy(wifiConfig.senha1,"fsjmr112");
  strcpy(wifiConfig.mqtt_host,"vemcompai.zapto.org");
  strcpy(wifiConfig.mqtt_port,"1883");
  strcpy(wifiConfig.clientId,"");
  strcpy(wifiConfig.mqtt_pass,"");
  strcpy(wifiConfig.subscribe_to,"test/topic");
  strcpy(wifiConfig.publish_to,"test/topic");
 
  wifiConfig.ip      = IPAddress(192,168,0,123);
  wifiConfig.gateway = IPAddress(192,168,0,1);
}

void setup() {
  
  escala.begin (PINO_DT, PINO_SCK); //inicializacao e definicao dos pinos DT e SCK dentro do objeto ESCALA

  Serial.begin(9600);

  preLoad();
  startWifiMode(ESP32_MODE_STA);
  
    
  xTaskCreatePinnedToCore(vMQTT,"vMQTT", 10000, NULL, 0, &handleStartMQTT,0);

  escala.tare(); //zera a escala
  escala.set_scale(fator_calibracao); //ajusta a escala para o fator de calibracao

}
 

void loop() {
  kg = escala.get_units(3); //retorna a leitura da variavel escala com a unidade quilogramas
  
  Serial.print(kg);
  Serial.println(" Kg");
  
  memset(buf,0,7);
  dtostrf(kg,sizeof(kg),3,buf);
  if (sendMsg == false){
    sendMsg = true;
  }
   
  vTaskDelay(pdMS_TO_TICKS(1000));
}

