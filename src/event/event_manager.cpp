#include "./event_manager.h"


EventManager::EventManager() 
{
    createSaveDir();
}

EventManager::~EventManager() 
{
    // Clean up
}

void EventManager::createSaveDir() 
{
    // Create directory if not exists
    mkdir(m_save_dir.c_str(), 0755);  
}

void EventManager::addEvent(const EventData& eventData) 
{
    Event event;
    event.timestamp = time(nullptr);
    event.type = eventData.type;
    event.thumbnail = saveFrame(eventData.frame, eventData.type);
    event.data = eventData.data;  // Copy metadata
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_history.push_back(event);
    m_event_queue.push(event);
    
    // 限制历史记录数量，防止内存溢出
    if(m_event_history.size() > m_max_history) {
        m_event_history.erase(m_event_history.begin());
    }
    
    printf("[EventManager] Event added: type=%s, thumbnail=%s (total: %zu)\n", 
           event.type.c_str(), event.thumbnail.c_str(), m_event_history.size());
}

std::string EventManager::saveFrame(const cv::Mat& frame, const std::string& type) 
{
    if(frame.empty()) {
        return "";
    }
    
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%s_%d_%ld.jpg", 
        m_save_dir.c_str(), type.c_str(), ++m_event_counter, time(nullptr));
    
    cv::imwrite(filename, frame);
    return filename;
}

std::vector<Event> EventManager::getRecentEvents(int count) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<Event> result;
    int start = (int)m_event_history.size() - count;
    if(start < 0) start = 0;
    for(int i = start; i < m_event_history.size(); i++) 
    {
        result.push_back(m_event_history[i]);
    }
    return result;
}

bool EventManager::hasNewEvent() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_event_queue.empty();
}

Event EventManager::popEvent() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if(m_event_queue.empty()) 
    {
        return Event();  // Return empty event
    }
    
    Event event = m_event_queue.front();
    m_event_queue.pop();
    return event;
}