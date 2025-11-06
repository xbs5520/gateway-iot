#ifndef __HTTP_SERVER_H
#define __HTTP_SERVER_H

#include <string>
#include <thread>
#include <atomic>
#include "../rtsp/rtsp_client.h"
#include "../event/event_manager.h"
#include "../../external/httplib.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace httplib {
    class Server;
}

class HTTPServer 
{
public:
    HTTPServer(int port = 8080);
    ~HTTPServer();
    
    void start(RTSPClient* client, EventManager* eventMgr);
    void stop();
    
private:
    void serverThread();
    void setupRoutes();
    
    int m_port;
    httplib::Server* m_server;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    
    RTSPClient* m_client;
    EventManager* m_eventMgr;
};

#endif //__HTTP_SERVER_H