#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "rtsp/rtsp_client.h"
#include "detector/motion_detector.h"
#include "event/event_manager.h"
#include "mqtt/mqtt_client.h"
#include "web/http_server.h"
#include "config/config.h"
// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

void signal_handler(int signal) 
{
    printf("\nReceived signal %d, shutting down...\n", signal);
    g_running = false;
}

int main(int argc, char* argv[])
{
    // Ctrl+C
    signal(SIGINT, signal_handler);

    Config& config = Config::getInstance();
    if (!config.load("./config.json")) 
    {
        printf("[Main] Warning: Failed to load config, using defaults\n");
    }

    std::string rtsp_url;
    if (argc >= 2) 
    {
        rtsp_url = argv[1];
    } 
    else 
    {
        json rtsp_config = config.getConfig("rtsp");
        if (!rtsp_config.is_null() && rtsp_config.contains("url") && rtsp_config["url"].is_string()) 
        {
            rtsp_url = rtsp_config["url"].get<std::string>();
        } 
        else 
        {
            rtsp_url = "rtsp://127.0.0.1:8554/test";
        }
    }

    RTSPClient client;
    MotionDetector detector;
    EventManager eventMgr;
    MQTTClient mqtt;
    HTTPServer httpServer(8080);

    // InitRTSP
    printf("Opening RTSP stream: %s\n", rtsp_url.c_str());
    if(!client.open(rtsp_url)) 
    {
        printf("Failed to open RTSP stream\n");
        return -1;
    }
    // connect AWS
    if(!mqtt.connect()) 
    {
        printf("MQTT connection failed\n");
    }
    // Start threads
    printf("Starting RTSP stream thread...\n");
    client.startStream();
    
    printf("Starting motion detector thread...\n");
    detector.startDetection(&client, &eventMgr);
    
    printf("Starting aws publish thread...\n");
    mqtt.startPublish(&eventMgr);

    printf("Starting HTTP server on port 8080...\n");
    httpServer.start(&client, &eventMgr);

    // Main loop
    int loop_count = 0;
    while(g_running) 
    {
        sleep(5);
        
        // Print stats every 5 seconds
        std::vector<Event> recent = eventMgr.getRecentEvents(5);
        printf("[Stats] Events: %zu total\n", recent.size());
        
        if(!recent.empty()) 
        {
            printf("  Latest event: type=%s, time=%ld\n", 
            recent.back().type.c_str(), 
            recent.back().timestamp);
        }

        loop_count++;
    }
    
    // Cleanup
    printf("\nStopping threads...\n");
    detector.stopDetection();
    client.stopStream();
    mqtt.stopPublish();
    httpServer.stop();
    
    printf("Shutdown complete\n");
    return 0;
}