# Gateway - Video Surveillance System

Smart video gateway with motion detection, cloud integration, and web interface.

## Features

✅ **RTSP Video Streaming** - Pull and decode video from IP cameras  
✅ **Motion Detection** - Real-time motion detection with configurable sensitivity  
✅ **Event Management** - Automatic event capture and storage  
✅ **MQTT Integration** - AWS IoT Core cloud messaging  
✅ **Web Dashboard** - Real-time video streaming and event monitoring  
✅ **Configuration System** - Dynamic config management with validation  

## Architecture

```
┌─────────────┐    RTSP     ┌──────────────┐
│ DJI Pocket3 ├────────────>│ RTSPClient   │
│  /IP Camera │             │  (FFmpeg)    │
└─────────────┘             └──────┬───────┘
                                   │ cv::Mat
                            ┌──────▼───────┐
                            │ MotionDetect │
                            │  (OpenCV)    │
                            └──────┬───────┘
                                   │ Events
                    ┌──────────────┼──────────────┐
                    ▼              ▼              ▼
            ┌───────────┐  ┌──────────┐  ┌──────────┐
            │  Event    │  │  MQTT    │  │  HTTP    │
            │  Manager  │  │  Client  │  │  Server  │
            └───────────┘  └────┬─────┘  └────┬─────┘
                 │ JPEG          │ AWS IoT    │ Web
                 ▼               ▼            ▼
            ./events/      Cloud Dashboard  Browser
```

## Module Overview

**RTSPClient** - Pulls video streams using FFmpeg, decodes to OpenCV Mat format, manages reconnection  
**MotionDetector** - Analyzes frames with MOG2 background subtraction, triggers events on motion  
**EventManager** - Captures snapshots on motion events, saves JPEG files with timestamps  
**MQTTClient** - Publishes event notifications to AWS IoT Core with TLS authentication  
**HTTPServer** - Serves web UI, provides REST API for events/config, streams video snapshots  
**Config** - Manages JSON configuration with validation callbacks and live updates

## Development Progress

### Stage 1: Core Pipeline ✅
- ✅ RTSP client with FFmpeg
- ✅ Motion detection (MOG2 algorithm)
- ✅ Event manager with JPEG storage
- ✅ MQTT client for AWS IoT Core
- ✅ HTTP server with REST API
- ✅ Web UI with live video streaming

### Stage 2: Configuration & Optimization ✅
- ✅ Universal configuration management system
- ✅ Signal-slot pattern for config callbacks
- ✅ Level-based motion sensitivity (1-10)
- ✅ Web-based configuration interface
- ✅ DJI Pocket 3 camera integration
- ✅ Fixed memory leaks (OOM issues resolved)

## Configuration

All settings managed via `config.json`:

```json
{
  "detector": {
    "motion_level": 5,      // Sensitivity 1-10
    "debounce": 3           // Seconds between events
  },
  "rtsp": {
    "url": "rtsp://127.0.0.1:8554/dji_camera",
    "reconnect_interval": 5
  },
  "http": {
    "port": 8080,
    "stream_quality": 70,
    "stream_fps": 10
  },
  "mqtt": {
    "enabled": true,
    "endpoint": "your-endpoint.iot.region.amazonaws.com",
    "port": 8883
  }
}
```

Or use web interface: `http://localhost:8080/config.html`

## Quick Start

### 1. Start RTSP Server
```bash
cd ~/Downloads
./mediamtx
```

### 2. Stream Camera
```bash
# For DJI Pocket 3 / USB Camera
cd Gateway/scripts
./dji_stream.sh

# Or use test video
cd RTSP_Streamer
ffmpeg -re -stream_loop -1 -i test.mp4 -f rtsp rtsp://127.0.0.1:8554/camera
```

### 3. Run Gateway
```bash
cd Gateway/build
cmake .. && make
./src/Gateway rtsp://127.0.0.1:8554/camera
```

### 4. Access Web UI
Open browser: `http://localhost:8080`

## Dependencies

- **FFmpeg** (libavformat, libavcodec, libavutil, libswscale)
- **OpenCV 4.x** (core, video, imgproc, imgcodecs)
- **cJSON** (configuration parsing)
- **cpp-httplib** (HTTP server)
- **AWS IoT SDK** (MQTT client)

## Key Improvements

**Memory Management:**
- Fixed buffer memory leak in `RTSPClient::convertFrameToMat()`
- Proper `av_free()` for all `av_malloc()` allocations
- Eliminated unnecessary Mat deep copies

**Configuration System:**
- Template-based wrapper for signal-slot pattern
- Change detection to avoid unnecessary updates
- Per-module validation callbacks
- Live configuration updates without restart

## Next Steps

- [ ] WebSocket streaming (reduce latency)
- [ ] Remote configuration via MQTT
- [ ] ARM cross-compilation for IMX6ULL
- [ ] Multi-camera support
- [ ] H.264 recording to disk