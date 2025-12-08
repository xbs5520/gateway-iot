#include <gtest/gtest.h>
#include "config/config.h"
#include <cjson/cJSON.h>

TEST(ConfigTest, LoadValidJson) 
{
    Config& cfg = Config::getInstance();
    EXPECT_TRUE(cfg.load("test_config.json"));
}

TEST(ConfigTest, GetConfig) 
{
    Config& cfg = Config::getInstance();
    cfg.load("test_config.json");
    
    cJSON* node = cfg.getConfig("test_section");
    EXPECT_NE(node, nullptr);
    
    cJSON* value = cJSON_GetObjectItem(node, "value");
    EXPECT_EQ(value->valueint, 2431);
}

TEST(ConfigTest, SetConfig) 
{
    Config& cfg = Config::getInstance();
    cfg.load("test_config.json");
    
    cJSON* new_config = cJSON_CreateObject();
    cJSON_AddNumberToObject(new_config, "value", 456);
    
    EXPECT_TRUE(cfg.setConfig("test_section", new_config));
    
    cJSON* updated = cfg.getConfig("test_section");
    cJSON* value = cJSON_GetObjectItem(updated, "value");
    EXPECT_EQ(value->valueint, 456);
    
    cJSON_Delete(new_config);
}

TEST(ConfigTest, SetAndSave) 
{
    Config& cfg = Config::getInstance();
    cfg.load("test_config.json");
    
    cJSON* new_config = cJSON_CreateObject();
    cJSON_AddNumberToObject(new_config, "value", 999);
    cfg.setConfig("test_section", new_config);
    
    // save
    EXPECT_TRUE(cfg.save());
    
    // check
    Config& cfg2 = Config::getInstance();
    cfg2.load("test_config.json");
    cJSON* reloaded = cfg2.getConfig("test_section");
    cJSON* value = cJSON_GetObjectItem(reloaded, "value");
    EXPECT_EQ(value->valueint, 999);
    
    cJSON_Delete(new_config);
}