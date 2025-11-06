#ifndef __RTSP_CLIENT_H
#define __RTSP_CLIENT_H

#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class RTSPClient 
{
public:
    RTSPClient();
    ~RTSPClient();

    bool open(std::string url);
    void startStream();
    void stopStream();
    cv::Mat getLatestFrame();
private:
    cv::Mat getFrame();
    void streamThread();

    // FFmpeg members
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    SwsContext *sws_ctx = nullptr;
    int m_video_stream_idx = -1;
    
    // Thread
    std::thread m_thread;
    std::mutex m_frame_mutex;
    cv::Mat m_latest_frame;
    std::atomic<bool> m_running{false};
};

#endif