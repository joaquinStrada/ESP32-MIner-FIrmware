#ifndef __MQTT_H__
#define __MQTT_H__

#include <Arduino.h>

class Mqtt {
    public:
        void setup (String nameMiner, String mqttUser,String mqttPass,String mqttTopic);
        bool init(void (*callback)(char*, uint8_t *, unsigned int));
        bool connected(void);
        void reconnect(void);
        void loop(void);
    private:
        String _nameMiner;
        String _mqttUser;
        String _mqttPass;
        String _mqttTopic;
        bool _isInitialized = false;
    protected:
};

#endif // __MQTT_H__