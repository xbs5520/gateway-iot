#ifndef __MQTT_CLIENT_H
#define __MQTT_CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <mosquitto.h>
#include "../event/event_manager.h"
#include "aws_config.h"
#include <unistd.h>
#include <cstring>
using std::string;

class MQTTClient 
{
public:
    MQTTClient();
    ~MQTTClient();
    
    bool connect();
    void startPublish(EventManager* eventMgr);
    void stopPublish();
    
private:
    void publishThread();
    void networkThread();
    std::string eventToJSON(const Event& event);
    
    struct mosquitto* m_mosq = nullptr;
    std::thread m_publish_thread;
    std::thread m_network_thread;
    std::atomic<bool> m_running{false};
    
    EventManager* m_eventMgr = nullptr;
    std::string m_topic = "gateway/events";
};

#endif