#ifndef __MOTION_DETECTOR_H
#define __MOTION_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <chrono>
#include <cjson/cJSON.h>
#include "../rtsp/rtsp_client.h"
#include "../event/event_manager.h"
#include "../config/config.h"
class MotionDetector 
{
public:
    MotionDetector();
    ~MotionDetector();
    
    void startDetection(RTSPClient* client, EventManager* eventMgr);
    void stopDetection();

    bool configVerifyDetector(cJSON* config);
    void onConfigDetector(cJSON* config);

private:
    void detectThread();
    int levelToThreshold(int level);

    cv::Ptr<cv::BackgroundSubtractorMOG2> m_bg_subtractor;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    
    RTSPClient* m_client;
    EventManager* m_eventMgr;
    
    int m_motion_threshold = 5000;
    int m_motion_level = 5;
    
    // jitter time
    int m_debounce_time = 3;
    
    std::chrono::steady_clock::time_point m_last_event_time;
};
 
#endif //__MOTION_DETECTOR_H