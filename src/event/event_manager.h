#ifndef __EVENT_MANAGER_H
#define __EVENT_MANAGER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <map>
#include <vector>
#include <queue>
#include <mutex>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

struct EventData 
{
    std::string type;                           // Event type
    cv::Mat frame;                              // Image frame
    std::map<std::string, std::string> data;    // Flexible key-value data
};

struct Event 
{
    int64_t timestamp;                          // Unix timestamp
    std::string type;                           // Event type
    std::string thumbnail;                      // Path to saved image
    std::map<std::string, std::string> data;    // Event data
};

class EventManager 
{
public:
    EventManager();
    ~EventManager();
    
    void addEvent(const EventData& eventData);
    std::vector<Event> getRecentEvents(int count = 10);

    bool hasNewEvent();     // Check if queue has events
    Event popEvent();       // Pop event for MQTT/Web consumption
    
private:
    std::string saveFrame(const cv::Mat& frame, const std::string& type);
    void createSaveDir();
    
    std::vector<Event> m_event_history;
    std::queue<Event> m_event_queue;
    std::mutex m_mutex;
    int m_max_history = 10;
    std::string m_save_dir = "./events";
    int m_event_counter = 0;
};

#endif