#include "./rtsp_client.h"

RTSPClient::RTSPClient()
{
    avformat_network_init();
}
RTSPClient::~RTSPClient()
{
    stopStream();
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
}

void RTSPClient::startStream() 
{
    m_running = true;
    m_thread = std::thread(&RTSPClient::streamThread, this);
}

void RTSPClient::streamThread() {
    while(m_running) 
    {
        cv::Mat frame = getFrame();
        
        if(!frame.empty()) 
        {
            std::lock_guard<std::mutex> lock(m_frame_mutex);
            m_latest_frame = frame;
        }
        // 小延迟防止CPU 100%
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

cv::Mat RTSPClient::getLatestFrame() 
{
    std::lock_guard<std::mutex> lock(m_frame_mutex);
    return m_latest_frame.clone();
}

void RTSPClient::stopStream() 
{
    m_running = false;
    if(m_thread.joinable()) 
    {
        m_thread.join();
    }
}

bool RTSPClient::open(std::string url)
{
    if(url.empty()) 
    {
        printf("RTSPClient::open empty !!\n");
        return false;
    }
    // Open RTSP stream
    if (avformat_open_input(&fmt_ctx, url.c_str(), NULL, NULL) < 0)
    {
        printf("avformat_open_input failed !!\n");
        return false;
    }
    // Find stream info
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        printf("avformat_find_stream_info failed !!\n");
        return false;
    }
    // Find video stream
    for (int i = 0; i < fmt_ctx->nb_streams; i++) 
    {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) 
        {
            m_video_stream_idx = i;
            break;
        }
    }

    // Find decoder
    const AVCodec *codec = avcodec_find_decoder(fmt_ctx->streams[m_video_stream_idx]->codecpar->codec_id);
    codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[m_video_stream_idx]->codecpar);
    avcodec_open2(codec_ctx, codec, NULL);

    // Init sws_scale (YUV -> BGR)
    sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_BGR24, SWS_BILINEAR, NULL, NULL, NULL);
    
    return true;
}

cv::Mat RTSPClient::getFrame() 
{
    AVPacket pkt;
    AVFrame *frame = av_frame_alloc();
    AVFrame *frame_bgr = av_frame_alloc();
    
    // Read packet
    while (av_read_frame(fmt_ctx, &pkt) >= 0)
    {
        // Just Video
        if (pkt.stream_index != m_video_stream_idx) 
        {
            av_packet_unref(&pkt);
            continue;
        }
        
        // send to Decode
        avcodec_send_packet(codec_ctx, &pkt);
        if (avcodec_receive_frame(codec_ctx, frame) == 0) 
        {
            // Allocate BGR buffer
            int size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, codec_ctx->width, codec_ctx->height, 1);
            uint8_t *buffer = (uint8_t*)av_malloc(size);
            av_image_fill_arrays(frame_bgr->data, frame_bgr->linesize, buffer, AV_PIX_FMT_BGR24, codec_ctx->width, codec_ctx->height, 1);
            
            // YUV -> BGR
            sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height, frame_bgr->data, frame_bgr->linesize);
            
            // Convert to cv::Mat (深拷贝避免悬空指针)
            cv::Mat mat(codec_ctx->height, codec_ctx->width, CV_8UC3, frame_bgr->data[0], frame_bgr->linesize[0]);
            cv::Mat result = mat.clone();
            
            // 释放所有资源
            av_free(buffer);
            av_packet_unref(&pkt);
            av_frame_free(&frame);
            av_frame_free(&frame_bgr);

            return result;
        }
        av_packet_unref(&pkt);
    }
    return cv::Mat();
}
