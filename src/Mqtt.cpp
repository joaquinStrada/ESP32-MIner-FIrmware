#include <Mqtt.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <config.h>
#include <ArduinoJson.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);



void Mqtt::setup(String nameMiner, String mqttUser,String mqttPass,String mqttTopic)
{
  _nameMiner = nameMiner;
  _mqttUser = mqttUser;
  _mqttPass = mqttPass;
  _mqttTopic = mqttTopic;
  _isInitialized = true;
}

bool Mqtt::init(void (*callback)(char*, uint8_t *, unsigned int))
{
  if (WiFi.status() != WL_CONNECTED || !_isInitialized)
  {
    return false;
  }
  
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
  return true;
}

bool Mqtt::connected(void)
{
  return mqttClient.connected();
}

void Mqtt::reconnect(void)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi desconectado!!!");
    WiFi.reconnect();
  }
  

  while (!mqttClient.connected())
  {
    Serial.print("intentando conexion con mqtt... ");

    // Generamos un clientId
    String clientId = _nameMiner + "_" + String(random(0xffff), HEX);

    // Nos intentamos conectar
    if (mqttClient.connect(clientId.c_str(), _mqttUser.c_str(), _mqttPass.c_str()))
    {
      Serial.println("Conectado!!!!");

      // Nos suscribimos
      String topicSubscribed = _mqttTopic + "/settings";

      if (mqttClient.subscribe(topicSubscribed.c_str()))
      {
        Serial.println("Subscripcion ok!!!");
      }
      else
      {
        Serial.println("Fallo la subscripcion!!!");
      }
      
    }
    else
    {
      Serial.print("Fallo con error: ");
      Serial.print(mqttClient.state());
      Serial.println(" Intentaremos de nuevo en 5 segundos");
      delay(5000);
    }   
  }
}

void Mqtt::loop(void)
{
  mqttClient.loop();
}

void Mqtt::send(StaticJsonDocument<128> doc)
{
  // paso de json a string
  String data = "";

  serializeJson(doc, data);

  // Mando por mqtt los datos
  mqttClient.publish(_mqttTopic.c_str(), (char*)data.c_str());
}



