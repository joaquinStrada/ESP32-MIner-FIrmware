#include <Arduino.h>
#include <config.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Miner.h>
#include <Mqtt.h>
#include <SPIFFS.h>
#include <utils.h>
#include "esp_heap_caps.h"

Miner miner;
Mqtt mqtt;

//************************
//** V A R I A B L E S ***
//************************
TaskHandle_t TaskSendData;

//************************
//** F U N C I O N E S ***
//************************
void setupWifi(void);
void setupMiner(void);
void sendData(void *parameter);
void callback(char * topic, uint8_t * payload, unsigned int length);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPIFFS.begin();
  setupWifi();
  setupMiner();

  xTaskCreatePinnedToCore(
    sendData,
    "sendData",
    10000,
    NULL,
    1,
    &TaskSendData,
    0
  );
}

void loop() {
  // put your main code here, to run repeatedly: 
  if (WiFi.status() == WL_CONNECTED)
  {
    miner.run();
  }
  else
  {
    WiFi.reconnect();
  }
}

//************************
//** S E T U P W I F I ***
//************************
void setupWifi() {
  delay(10);

  // Nos conectamos al wifi
  Serial.println("WiFi Setup");
  Serial.print("Conectando a SSID: ");
  Serial.print(WiFi_SSID);

  WiFi.begin(WiFi_SSID, WiFi_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("");
  Serial.println("Conectado a red WiFI!!!");
  Serial.print("Direccion Ip: ");
  Serial.println(WiFi.localIP());
}

//************************
//** S E T U P M I N E R ***
//************************
void setupMiner() {
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("No estas conectado a la red wifi!!!");
    return;
  }
  
  // Hacemos una peticion al servidor
  Serial.println("Obteniendo los datos del minero");
  HTTPClient http;
  String data_send = "serie=" + String(SERIE_MINER) + "&password=" + String(PASS_MINER);

  http.begin(API_LOGIN);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code_response = http.POST(data_send);

  if (code_response <= 0)
  {
    Serial.print("Error enviando post: ");
    Serial.println(code_response);
    return;
  } else if (code_response != 200) {
    Serial.print("Error http post: ");
    Serial.println(code_response);
    return;
  }
  
  String response = http.getString();

  Serial.print("Response: ");
  Serial.println(response);

  // Parseando los datos
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);
  
  if (error)
  {
    Serial.print("Error parseando los datos: ");
    Serial.println(error.c_str());
    return;
  } else if (doc["error"]) {
    Serial.print("Error http post: ");
    Serial.println(String((const char *)doc["message"]));
  }
  
  JsonObject data = doc["data"];

  String nameMiner = String((const char *)data["name"]);
  String poolUrl = String((const char *)data["poolUrl"]);
  int poolPort = data["poolPort"];
  String walletAddress = String((const char *)data["walletAddress"]);
  String mqttUser = String((const char *)data["mqttUser"]); 
  String mqttPass = String((const char *)data["mqttPassword"]); 
  String mqttTopic = String((const char *)data["mqttTopic"]);

  // Mostramos los datos por pantalla
  Serial.print("Nombre del minero: ");
  Serial.println(nameMiner);
  Serial.print("Url de la pool: ");
  Serial.println(poolUrl);
  Serial.print("Puerto de la pool: ");
  Serial.println(poolPort);
  Serial.print("Direccion de la wallet: ");
  Serial.println(walletAddress);
  Serial.print("Usuario de mqtt: ");
  Serial.println(mqttUser);
  Serial.print("Contraseña de mqtt: ");
  Serial.println(mqttPass);
  Serial.print("Topico de mqtt: ");
  Serial.println(mqttTopic);

  // Inicializamos los objetos
  miner.setup(nameMiner, poolUrl, poolPort, walletAddress);
  mqtt.setup(nameMiner, mqttUser, mqttPass, mqttTopic);
}

//************************
//** S E N D D A T A ***
//************************
void sendData(void *parameter) {
  mqtt.init(callback);

  while (true)
  {
    if (!mqtt.connected())
    {
      mqtt.reconnect();
    }
    
    if (mqtt.connected())
    {
      // Creamos el json con los datos
      StaticJsonDocument<128> doc;
      long unsigned int validShares = 0;
      long unsigned int invalidShares = 0;
      long unsigned int hashrate = 0;
      float memory = 0;
      float memoryPsram = 0;
      float disk = 0;
      float red = 0;

      for (int i = 0; i < SAMPLES; i++)
      {
        validShares += miner.getData(VALID_SHARES);
        invalidShares += miner.getData(INVALID_SHARES);
        hashrate += miner.getData(HASHRATE);

        size_t heapSize = ESP.getHeapSize();
        size_t freeHeap = ESP.getFreeHeap();
        size_t psramSize = ESP.getPsramSize();
        size_t freePsram = ESP.getFreePsram();
        size_t totalDisk = SPIFFS.totalBytes();
        size_t usedDisk = SPIFFS.usedBytes();

        memory += (heapSize > 0) ? (100.0 - ((float)freeHeap * 100.0 / (float)heapSize)) : 0;
        memoryPsram += (psramSize > 0) ? (100 - ((float)freePsram * 100.0 / (float)psramSize)) : 0;
        disk += (totalDisk > 0) ? ((float)usedDisk * 100.0 / (float)totalDisk) : 0;
        red += getRSSIAsQuality(WiFi.RSSI());
      }
      
      doc["validShares"] = validShares/SAMPLES;
      doc["invalidShares"] = invalidShares/SAMPLES;
      doc["memory"] = roundNumber(memory/(float)SAMPLES, 2);
      doc["memoryPsram"] = roundNumber(memoryPsram/(float)SAMPLES, 2);
      doc["disk"] = roundNumber(disk/(float)SAMPLES, 2);
      doc["red"] = roundNumber(red/(float)SAMPLES, 2);
      doc["hashrate"] = hashrate/SAMPLES;

      // Mandamos los datos por mqtt
      mqtt.send(doc);
      delay(300);
    }
    

    mqtt.loop();
  }
  vTaskDelay(10);
}

//************************
//** C A L L B A C K ***
//************************
void callback(char * topic, uint8_t * payload, unsigned int length) {

}

