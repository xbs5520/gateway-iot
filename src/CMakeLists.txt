#rtsp
add_library(rtsp rtsp/rtsp_client.cpp)
target_include_directories(rtsp PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)
target_link_libraries(rtsp ${OpenCV_LIBS} ${FFMPEG_LIBRARIES})

#detector
add_library(detector detector/motion_detector.cpp)
target_include_directories(detector PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(detector ${OpenCV_LIBS})

#config
add_library(config config/config.cpp)
target_include_directories(config PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CJSON_INCLUDE_DIRS}
)
target_link_libraries(config ${CJSON_LIBRARIES})

#event
add_library(event event/event_manager.cpp)
target_include_directories(event PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(event ${OpenCV_LIBS})

#mqtt
add_library(mqtt mqtt/mqtt_client.cpp)
target_include_directories(mqtt PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
    ${MOSQUITTO_INCLUDE_DIRS}
)
target_link_libraries(mqtt ${MOSQUITTO_LIBRARIES} ${CJSON_LIBRARIES} )

#web
add_library(web web/http_server.cpp)
target_include_directories(web PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/external/cpp-httplib
)
target_link_libraries(web 
    ${OpenCV_LIBS}
    pthread
)
add_executable(Gateway main.cpp)

target_link_libraries(Gateway
    config
    rtsp
    mqtt
    detector
    event
    web
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    ${MOSQUITTO_LIBRARIES}
)

# cert
file(COPY ${CMAKE_SOURCE_DIR}/cert
     DESTINATION ${CMAKE_BINARY_DIR})

# web静态文件
file(COPY ${CMAKE_SOURCE_DIR}/web
     DESTINATION ${CMAKE_BINARY_DIR})