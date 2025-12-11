#include "./motion_detector.h"

MotionDetector::MotionDetector()
{
    m_bg_subtractor = cv::createBackgroundSubtractorMOG2();

    // add onconfig && verify
    Config::getInstance().registerVerify("detector", this, &MotionDetector::configVerifyDetector);
    Config::getInstance().registerOnConfig("detector", this, &MotionDetector::onConfigDetector);
    
    // init config
    json config = Config::getInstance().getConfig("detector");
    if (!config.is_null()) 
    {
        onConfigDetector(config);  // init
    } 
    else 
    {
        // default
        printf("[Detector] No config found, using defaults: level=%d, debounce=%ds\n", m_motion_level, m_debounce_time);
    }

    // make sure first time detect ok
    m_last_event_time = std::chrono::steady_clock::now() - std::chrono::seconds(3600);
}

MotionDetector::~MotionDetector()
{
    stopDetection();
    // remove onconfig && verify
    Config::getInstance().removeVerify("detector", this, &MotionDetector::configVerifyDetector);
    Config::getInstance().removeOnConfig("detector", this, &MotionDetector::onConfigDetector);
}

int MotionDetector::levelToThreshold(int level)
{
    if (level < 1) level = 1;
    if (level > 10) level = 10;
    
    return 100000 / (1 << (level - 1));  // 100000 / 2^(level-1)
}

void MotionDetector::startDetection(RTSPClient* client, EventManager* eventMgr) 
{
    m_client = client;
    m_eventMgr = eventMgr;
    m_running = true;
    m_thread = std::thread(&MotionDetector::detectThread, this);
}
void MotionDetector::stopDetection()
{
    m_running = false;
    if(m_thread.joinable()) 
    {
        m_thread.join();
    }
}

void MotionDetector::detectThread() 
{
    while(m_running) 
    {
        cv::Mat frame = m_client->getLatestFrame();
        if(frame.empty()) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        cv::Mat fg_mask;
        m_bg_subtractor->apply(frame, fg_mask);
        
        int motion_pixels = cv::countNonZero(fg_mask);
        if(motion_pixels > m_motion_threshold) 
        {
            // check jitter
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_event_time).count();
            
            if(elapsed / 1000 >= m_debounce_time) 
            {
                printf("Motion detected! pixels=%d\n", motion_pixels);
                EventData event;
                event.type = "motion";
                event.frame = frame;
                event.data["pixels"] = std::to_string(motion_pixels);
                event.data["threshold"] = std::to_string(m_motion_threshold);
                
                m_eventMgr->addEvent(event);
                m_last_event_time = now;  // 更新上次事件时间
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool MotionDetector::configVerifyDetector(const json& config)
{
    if (config.is_null()) 
    {
        printf("[Detector] Config is null\n");
        return false;
    }
    
    // verify motion_level
    if (config.contains("motion_level") && config["motion_level"].is_number()) 
    {
        int val = config["motion_level"].get<int>();
        if (val < 1 || val > 10) 
        {
            printf("[Detector] level out of range: %d (valid: 1-10)\n", val);
            return false;
        }
    }
    
    // verify debounce
    if (config.contains("debounce") && config["debounce"].is_number()) 
    {
        int val = config["debounce"].get<int>();
        if (val < 0 || val > 60) {
            printf("[Detector] Debounce out of range: %d (valid: 0-60s)\n", val);
            return false;
        }
    }
    
    printf("[Detector] Config verification passed\n");
    return true;
}

void MotionDetector::onConfigDetector(const json& config)
{
    if (config.is_null()) {
        printf("[Detector] Config is null\n");
        return;
    }
    
    // motion_threshold
    if (config.contains("motion_level") && config["motion_level"].is_number()) 
    {
        m_motion_level = config["motion_level"].get<int>();
        m_motion_threshold = levelToThreshold(m_motion_level);
        printf("[Detector] Updated motion_level: %d\n", m_motion_level);
    }

    // debounce
    if (config.contains("debounce") && config["debounce"].is_number()) 
    {
        m_debounce_time = config["debounce"].get<int>();
        printf("[Detector] Updated debounce_time: %d\n", m_debounce_time);
    }
}