 ![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg) ![License](https://img.shields.io/badge/license-MIT-green.svg)![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg) ![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)![Version](https://img.shields.io/badge/version-1.0.0-orange.svg)

# Gateway - Video Surveillance System

Gateway IoT is a C++17-based smart video surveillance system that enables real-time motion detection, event capture, and cloud integration for IP cameras and edge devices.

This project helps you quickly build a scalable, configurable, and cloud-ready video gateway for smart home, industrial monitoring, or IoT edge scenarios. Designed for easy integration with AWS IoT and web dashboards

## Demo



## Features

✅ **RTSP Video Streaming** - Pull and decode video from IP cameras 
✅ **Motion Detection** - Real-time motion detection with configurable sensitivity 
✅ **Event Management** - Automatic event capture and storage 
✅ **MQTT Integration** - AWS IoT Core cloud messaging 
✅ **Web Dashboard** - Real-time video streaming and event monitoring 
✅ **Configuration System** - Dynamic config management with validation  

## Why This Project?

- Existing open-source video gateways are often hard to configure, lack cloud integration, or are not optimized for edge devices.
- I needed a lightweight, modular, and easily extensible system for real-time video analytics and event-driven IoT applications.
- This project provides a modern C++17 codebase with clear architecture, dynamic configuration, and seamless integration with AWS IoT and web dashboards.
- 

## Architecture

### Configuration flow

<img src="readme/configflow.png" alt="config flow" style="zoom: 67%;" />

### Network flow

<img src="readme/Networkflow.png" alt="Network flow" style="zoom: 67%;" />

### Stream flow

<img src="readme/streamflow.png" alt="stream flow" style="zoom: 67%;" />



### Event flow

**motion detect**

<img src="readme/Motiondetectflow.png" alt="Motion detect flow" style="zoom: 67%;" />

## Module Overview

**RTSPClient** - Pulls video streams using FFmpeg, decodes to OpenCV Mat format, manages reconnection 
**MotionDetector** - Analyzes frames with MOG2 background subtraction, triggers events on motion 
**EventManager** - Captures snapshots on motion events, saves JPEG files with timestamps 
**MQTTClient** - Publishes event notifications to AWS IoT Core with TLS authentication 
**HTTPServer** - Serves web UI, provides REST API for events/config, streams video snapshots 
**Config** - Manages JSON configuration with validation callbacks and live updates

## System Requirements

- **Hardware:**
  - CPU: x86_64
  - RAM: Minimum 512MB (1GB+ recommended)
  - Storage: Minimum 100MB (excluding event images)
  - Camera: RTSP-compatible IP camera or USB camera (UVC)
- **Software:**
  - OS: Linux (tested on Ubuntu 20.04+, Debian)
  - Kernel: V4L2 support required (for USB cameras)
  - Compiler: GCC 7+ (C++17 support)
  - CMake: 3.10 or higher
- **Dependencies:**
  - FFmpeg 4.x+ (libavformat, libavcodec, libavutil, libswscale)
  - OpenCV 4.x (core, video, imgproc, imgcodecs)
  - cJSON (latest)
  - Mosquitto 2.0+ (MQTT client library)
  - OpenSSL 1.1+ (for TLS support)
- **Optional:**
  - AWS IoT certificates (for cloud integration)
  - mediamtx (RTSP server for testing)
- **Network Ports:**
  - HTTP: 8080 (web interface)
  - MQTT: 8883 (TLS, AWS IoT Core)

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

## Installation

### 1.Install Dependencies

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    libavformat-dev libavcodec-dev libavutil-dev libswscale-dev \
    libopencv-dev \
    libcjson-dev \
    libmosquitto-dev \
    libssl-dev

# Verify installations
ffmpeg -version
pkg-config --modversion opencv4
```

### 2. Build Project
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. Stream Camera
```bash
# For DJI Pocket 3 / USB Camera
ffmpeg -f v4l2 -video_size 1280x720 -framerate 30 -i /dev/video0 \
    -c:v libx264 -preset ultrafast -f rtsp rtsp://127.0.0.1:8554/camera

# Or use test video
cd RTSP_Streamer
ffmpeg -re -stream_loop -1 -i test.mp4 -f rtsp rtsp://127.0.0.1:8554/camera
```

### 4. Run Gateway
```bash
./src/Gateway rtsp://127.0.0.1:8554/camera
```

### 5. Access Web UI
Open browser: `http://localhost:8080`

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

## Troubleshooting

### Problem: Out of Memory (OOM)

Solution:
- Already fixed in current version (av_free added)
- Limit event history in config.json: "max_history": 100
- Monitor memory: watch -n 1 free -h

### Problem: Motion detection too sensitive/insensitive

Solution:
- Adjust motion_level in config.json (1-10)
- Level 1 = least sensitive, Level 10 = most sensitive
- Or use web UI: http://localhost:8080/config.html

### Problem: MQTT connection failed

Solution:
- Check AWS IoT endpoint is correct
- Verify certificates exist in ./cert/ directory
- Test with: mosquitto_pub (with same certs)
- Check port 8883 is not blocked by firewall

### Problem: Permission denied on /dev/video0

Solution:
- Temporary: sudo chmod 666 /dev/video0
- Permanent: sudo usermod -a -G video $USER (then re-login)

## Roadmap

**Planned Features:**

-  **Enhanced Web Configuration** - More granular settings for detection, networking, and storage
-  **Face Recognition** - Integrate face detection and recognition using OpenCV DNN or dlib
-  **Database Integration** - Store events and analytics in PostgreSQL/MySQL
-  **Web UI Improvements** - Real-time WebSocket streaming, responsive design, dashboard charts
-  **Extended AWS Integration** - CloudWatch metrics, Lambda triggers, S3 image storage
-  **OTA Updates** - Remote firmware/configuration updates via MQTT
-  **Multi-camera Support** - Handle multiple video sources simultaneously
-  **H.264 Recording** - Continuous or event-triggered video recording to disk
-  **ARM Cross-compilation** - Optimized build for IMX6ULL and Raspberry Pi
-  **Alert Notifications** - Email/SMS/push notifications on events
-  **AI Analytics** - Object detection, people counting, behavior analysis

**Contributions welcome!**

## Contributing

Contributions are welcome! Here's how you can help:

**Ways to Contribute:**

- 🐛 Report bugs and issues
- 💡 Suggest new features or improvements
- 📝 Improve documentation
- 🔧 Submit pull requests with bug fixes or features
- ⭐ Star the project if you find it useful

**Development Process:**

1. Fork the repository
2. Create your feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to the branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

**Code Style:**

- Follow existing C++17 code conventions
- Use meaningful variable/function names
- Add comments for complex logic
- Test your changes before submitting

**Pull Request Guidelines:**

- Describe what your PR does and why
- Reference any related issues
- Ensure code compiles without warnings
- Update documentation if needed