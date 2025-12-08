add_executable(config_test test_config.cpp)
target_link_libraries(config_test GTest::GTest GTest::Main config)
gtest_discover_tests(config_test)
configure_file(test_config.json ${CMAKE_CURRENT_BINARY_DIR}/test_config.json COPYONLY)
