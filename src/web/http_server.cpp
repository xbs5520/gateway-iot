#include "http_server.h"
#include "../config/config.h"

HTTPServer::HTTPServer(int port) : m_port(port), m_server(nullptr)
{
    m_server = new httplib::Server();
}

HTTPServer::~HTTPServer()
{
    stop();
    if(m_server) {
        delete m_server;
    }
}

void HTTPServer::start(RTSPClient* client, EventManager* eventMgr)
{
    m_client = client;
    m_eventMgr = eventMgr;
    m_running = true;
    
    setupRoutes();
    
    m_thread = std::thread(&HTTPServer::serverThread, this);
    printf("[HTTPServer] Started on port %d\n", m_port);
    printf("[HTTPServer] Open http://localhost:%d in your browser\n", m_port);
}

void HTTPServer::stop()
{
    m_running = false;
    if(m_server) {
        m_server->stop();
    }
    if(m_thread.joinable()) {
        m_thread.join();
    }
}

void HTTPServer::serverThread()
{
    m_server->listen("0.0.0.0", m_port);
}

void HTTPServer::setupRoutes()
{
    // 1. set static dir /html /css /js
    m_server->set_mount_point("/", "./web");

    // 2. snapshot API
    m_server->Get("/snapshot.jpg", [this](const httplib::Request&, httplib::Response& res) 
    {
        // mutex？
        cv::Mat frame = m_client->getLatestFrame();
        if(!frame.empty()) 
        {
            cv::Mat resized;
            cv::resize(frame, resized, cv::Size(640, 480));
            
            std::vector<uchar> buf;
            std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 70};
            cv::imencode(".jpg", resized, buf, params);
            
            res.set_content((char*)buf.data(), buf.size(), "image/jpeg");
        } 
        else 
        {
            res.status = 503;
            res.set_content("No frame available", "text/plain");
        }
    });
    
    // 3. events
    m_server->Get("/api/events", [this](const httplib::Request&, httplib::Response& res) 
    {
        std::string json = "[";
        namespace fs = std::filesystem;
        if(fs::exists("./events"))  // 如果events目录存在
        {
            bool first = true;
            // 遍历events目录下的所有文件
            for(const auto& entry : fs::directory_iterator("./events")) 
            {
                // 只处理.jpg文件
                if(entry.path().extension() == ".jpg") 
                {
                    if(!first) json += ",";  // 不是第一个就加逗号
                    // add filename to json
                    std::string filename = entry.path().filename().string();
                    json += "{\"filename\":\"" + filename + "\"}";
                    first = false;
                }
            }
        }
        json += "]";  // json end
        
        // 返回JSON字符串，Content-Type设置为application/json
        res.set_content(json, "application/json");
    });
    
    // 4. thumbnail
    m_server->Get("/api/thumbnail/(.*)", [this](const httplib::Request& req, httplib::Response& res) 
    {
        // req.matches[1] 是正则匹配到的文件名部分
        // 例如 /api/thumbnail/motion_1.jpg，matches[1] = "motion_1.jpg"
        std::string filename = req.matches[1];
        std::string filepath = "./events/" + filename;
        
        // 以二进制方式打开文件
        std::ifstream file(filepath, std::ios::binary);
        if(file.is_open())  // 文件存在
        {
            // 把整个文件读到buffer
            std::stringstream buffer;
            buffer << file.rdbuf();
            
            // 返回图片数据，Content-Type是image/jpeg
            res.set_content(buffer.str(), "image/jpeg");
        } 
        else  // 文件不存在
        {
            res.status = 404;  // HTTP 404 Not Found
            res.set_content("Image not found", "text/plain");
        }
    });

    // 5. API: 获取事件个数
    m_server->Get("/api/stats", [this](const httplib::Request&, httplib::Response& res) 
    {
        // 用 getRecentEvents
        auto events = m_eventMgr->getRecentEvents(10);  // 获取最多10个
        
        std::string json = "{";
        json += "\"total\": " + std::to_string(events.size());
        json += "}";
        
        res.set_content(json, "application/json");
    });

    // 6. API: 获取完整配置
    m_server->Get("/api/config", [this](const httplib::Request&, httplib::Response& res) 
    {
        Config& cfg = Config::getInstance();
        cJSON* root = cfg.getRoot();
        if(root) {
            char* json_str = cJSON_Print(root);
            res.set_content(json_str, "application/json");
            free(json_str);
        } else {
            res.status = 500;
            res.set_content("{\"error\":\"Failed to get config\"}", "application/json");
        }
    });

    // 7. API: 更新配置
    m_server->Post("/api/config", [this](const httplib::Request& req, httplib::Response& res) 
    {
        Config& cfg = Config::getInstance();
        
        // 解析POST的JSON数据
        cJSON* new_config = cJSON_Parse(req.body.c_str());
        if(!new_config) {
            res.set_content("{\"success\":false,\"error\":\"Invalid JSON\"}", "application/json");
            return;
        }
        
        bool all_success = true;
        std::string error_msg;
        
        // 遍历每个配置项并更新
        cJSON* item = new_config->child;
        while(item) {
            std::string name = item->string;
            cJSON* value = cJSON_Duplicate(item, 1);  // 深拷贝
            
            if(!cfg.setConfig(name, value)) {
                all_success = false;
                error_msg += name + " 验证失败; ";
                cJSON_Delete(value);
            }
            item = item->next;
        }
        
        cJSON_Delete(new_config);
        
        // 保存到文件
        if(all_success) {
            cfg.save();
            res.set_content("{\"success\":true}", "application/json");
        } else {
            std::string json = "{\"success\":false,\"error\":\"" + error_msg + "\"}";
            res.set_content(json, "application/json");
        }
    });
}