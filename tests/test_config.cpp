#include <gtest/gtest.h>
#include "config/config.h"
#include "../external/json.hpp"

using json = nlohmann::json;

TEST(ConfigTest, LoadValidJson) 
{
    Config& cfg = Config::getInstance();
    EXPECT_TRUE(cfg.load("test_config.json"));
}

TEST(ConfigTest, GetConfig) 
{
    Config& cfg = Config::getInstance();
    cfg.load("test_config.json");
    
    json node = cfg.getConfig("test_section");
    EXPECT_FALSE(node.is_null());
    
    EXPECT_TRUE(node.contains("value"));
    EXPECT_EQ(node["value"].get<int>(), 2431);
}

TEST(ConfigTest, SetConfig) 
{
    Config& cfg = Config::getInstance();
    cfg.load("test_config.json");
    
    json new_config = {{"value", 456}};
    
    EXPECT_TRUE(cfg.setConfig("test_section", new_config));
    
    json updated = cfg.getConfig("test_section");
    EXPECT_TRUE(updated.contains("value"));
    EXPECT_EQ(updated["value"].get<int>(), 456);
}

TEST(ConfigTest, SetAndSave) 
{
    Config& cfg = Config::getInstance();
    cfg.load("test_config.json");
    
    json new_config = {{"value", 999}};
    cfg.setConfig("test_section", new_config);
    
    // save
    EXPECT_TRUE(cfg.save());
    
    // check
    Config& cfg2 = Config::getInstance();
    cfg2.load("test_config.json");
    json reloaded = cfg2.getConfig("test_section");
    EXPECT_TRUE(reloaded.contains("value"));
    EXPECT_EQ(reloaded["value"].get<int>(), 999);
}