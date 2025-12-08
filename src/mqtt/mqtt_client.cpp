#include "./mqtt_client.h"

MQTTClient::MQTTClient()
{
    mosquitto_lib_init();
}
MQTTClient::~MQTTClient()
{
    stopPublish();
    // Stop network loop
    mosquitto_disconnect(m_mosq);
    mosquitto_destroy(m_mosq);
    mosquitto_lib_cleanup();
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) 
{
    if (rc == 0) 
    {
        printf("Connected to AWS IoT successfully\n");
        
        // Subscribe to command topic
        int sub_rc = mosquitto_subscribe(mosq, NULL, AWS_TOPIC_PUBLISH, 0);
        if (sub_rc == MOSQ_ERR_SUCCESS) 
        {
            printf("Subscribed to command topic\n");
        } else 
        {
            fprintf(stderr, "Subscribe failed: %s\n", mosquitto_strerror(sub_rc));
        }
    } 
    else 
    {
        fprintf(stderr, "Connection failed: %s\n", mosquitto_strerror(rc));
    }
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc) 
{
    if (rc == 0) 
    {
        printf("✓ Disconnected normally\n");
    } 
    else 
    {
        fprintf(stderr, "✗ Connection lost unexpectedly: %s\n", mosquitto_strerror(rc));
    }
}


bool MQTTClient::connect()
{
    // Create client instance
    m_mosq = mosquitto_new(AWS_IOT_CLIENT_ID, true, NULL);
    if (!m_mosq) 
    {
        fprintf(stderr, "Failed to create client\n");
        return false;
    }
    
    // Set protocol version to MQTT 3.1.1
    mosquitto_int_option(m_mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V311);
    
    // Set callback functions
    mosquitto_connect_callback_set(m_mosq, on_connect);
    mosquitto_disconnect_callback_set(m_mosq, on_disconnect);
    
    int rc = mosquitto_tls_set(m_mosq, AWS_CERT_CA, NULL, AWS_CERT_CRT, AWS_CERT_KEY, NULL);
    
    if (rc != MOSQ_ERR_SUCCESS) 
    {
        fprintf(stderr, "TLS configuration failed: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(m_mosq);
        mosquitto_lib_cleanup();
        return false;
    }
    
    // Connect to AWS IoT
    printf("Connecting to AWS IoT...\n");
    rc = mosquitto_connect(m_mosq, AWS_IOT_ENDPOINT, AWS_IOT_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) 
    {
        fprintf(stderr, "Connection failed: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(m_mosq);
        mosquitto_lib_cleanup();
        return false;
    }
    return true;
}

void MQTTClient::startPublish(EventManager* eventMgr)
{
    m_eventMgr = eventMgr;
    m_running = true;
    m_publish_thread = std::thread(&MQTTClient::publishThread, this);
    m_network_thread = std::thread(&MQTTClient::networkThread, this);
}

void MQTTClient::stopPublish()
{
    m_running = false;
    
    if(m_network_thread.joinable()) 
    {
        m_network_thread.join();
    }
    if(m_publish_thread.joinable()) 
    {
        m_publish_thread.join();
    }
}

std::string MQTTClient::eventToJSON(const Event& event)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", event.type.c_str());
    cJSON_AddNumberToObject(root, "timestamp", event.timestamp);
    cJSON_AddStringToObject(root, "thumbnail", event.thumbnail.c_str());
    cJSON *data = cJSON_CreateObject();
    for(const auto& [key, value] : event.data)
    {
        cJSON_AddStringToObject(data, key.c_str(), value.c_str());
    }
    cJSON_AddItemToObject(root, "data", data);
    // memory leak
    // string res = cJSON_PrintUnformatted(root);
    char* json_str = cJSON_PrintUnformatted(root);
    std::string res(json_str);
    free(json_str);
    cJSON_Delete(root);
    return res;
}

void MQTTClient::publishThread()
{
    while(m_running)
    {
        if(m_eventMgr->hasNewEvent())
        {
            Event event = m_eventMgr->popEvent();
            std::string message = eventToJSON(event);

            // Publish to AWS IoT
            int rc = mosquitto_publish(m_mosq, NULL, AWS_TOPIC_PUBLISH, strlen(message.c_str()), message.c_str(), 0, false);
            
            if (rc == MOSQ_ERR_SUCCESS) 
            {
                printf("Publish success %s\n", message.c_str());
            } 
            else 
            {
                fprintf(stderr, "Publish failed: %s (rc=%d)\n", mosquitto_strerror(rc), rc);
            }
        } 
        // Wait 1 seconds
        sleep(1);
    }
}

void MQTTClient::networkThread() 
{
    while (m_running) 
    {
        int rc = mosquitto_loop(m_mosq, 1000, 1);
        if (rc != MOSQ_ERR_SUCCESS) 
        {
            fprintf(stderr, "Network error: %s\n", mosquitto_strerror(rc));
            if (rc == MOSQ_ERR_NO_CONN || rc == MOSQ_ERR_CONN_LOST) 
            {
                printf("Reconnecting...\n");
                mosquitto_reconnect(m_mosq);
                sleep(3);
            } 
            else 
            {
                break;
            }
        }
    }
}