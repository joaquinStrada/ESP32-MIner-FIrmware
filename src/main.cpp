#include <Arduino.h>
#include <config.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Miner.h>
#include <Mqtt.h>

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
    delay(100);
    mqtt.loop();
  }
  vTaskDelay(10);
}

//************************
//** C A L L B A C K ***
//************************
void callback(char * topic, uint8_t * payload, unsigned int length) {

}

