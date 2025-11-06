#include "config.h"
#include <fstream>
#include <sstream>
#include <cstring>

Config& Config::getInstance() 
{
    static Config instance;
    return instance;
}

Config::~Config() 
{
    if (m_root) {
        cJSON_Delete(m_root);
    }
    
    // 清理包装器
    for (auto& pair : m_verify_funcs) 
    {
        for (auto wrapper : pair.second) 
        {
            delete wrapper;
        }
    }
    
    for (auto& pair : m_callback_funcs) 
    {
        for (auto wrapper : pair.second) 
        {
            delete wrapper;
        }
    }
}

bool Config::load(const std::string& file_path) 
{
    m_file_path = file_path;
    
    std::ifstream file(file_path);
    if (!file.is_open()) 
    {
        printf("[Config] File not found: %s, creating empty config\n", file_path.c_str());
        m_root = cJSON_CreateObject();
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    if (m_root) 
    {
        cJSON_Delete(m_root);
    }
    
    m_root = cJSON_Parse(content.c_str());
    if (!m_root) {
        printf("[Config] Parse error: %s\n", cJSON_GetErrorPtr());
        m_root = cJSON_CreateObject();
        return false;
    }
    
    printf("[Config] Loaded from %s\n", file_path.c_str());
    return true;
}

bool Config::save() 
{
    if (!m_root) return false;
    
    char* json_str = cJSON_Print(m_root);
    if (!json_str) return false;
    
    std::ofstream file(m_file_path);
    if (!file.is_open()) {
        free(json_str);
        printf("[Config] Failed to save to %s\n", m_file_path.c_str());
        return false;
    }
    
    file << json_str;
    file.close();
    free(json_str);
    
    printf("[Config] Saved to %s\n", m_file_path.c_str());
    return true;
}

cJSON* Config::getConfig(const std::string& name) 
{
    if (!m_root) return nullptr;
    return cJSON_GetObjectItem(m_root, name.c_str());
}

bool Config::setConfig(const std::string& name, cJSON* config) 
{
    if (!m_root || !config) {
        printf("[Config] Invalid parameters\n");
        return false;
    }
    
    // 0. 检查配置是否真的改变了
    cJSON* old_config = cJSON_GetObjectItem(m_root, name.c_str());
    if (old_config) 
    {
        char* old_str = cJSON_PrintUnformatted(old_config);
        char* new_str = cJSON_PrintUnformatted(config);
        
        bool is_same = (strcmp(old_str, new_str) == 0);
        
        free(old_str);
        free(new_str);
        
        if (is_same) 
        {
            printf("[Config] '%s' not changed, skip update\n", name.c_str());
            return true;  // 没变化，直接返回成功
        }
    }
    
    // 1. 执行所有验证函数（全部通过才更新）
    auto verify_it = m_verify_funcs.find(name);
    if (verify_it != m_verify_funcs.end()) {
        for (auto wrapper : verify_it->second) {
            if (!wrapper->call(config)) {
                printf("[Config] Verification failed for '%s'\n", name.c_str());
                return false;
            }
        }
    }
    
    // 2. 更新配置
    cJSON_DeleteItemFromObject(m_root, name.c_str());
    cJSON_AddItemToObject(m_root, name.c_str(), cJSON_Duplicate(config, 1));
    
    // 3. 触发所有回调（只有真正改变才触发）
    auto callback_it = m_callback_funcs.find(name);
    if (callback_it != m_callback_funcs.end()) {
        for (auto wrapper : callback_it->second) {
            wrapper->call(config);
        }
    }
    
    printf("[Config] Updated '%s'\n", name.c_str());
    return true;
}

void Config::registerVerify(const std::string& name, bool (*verify_func)(cJSON*))
{
    m_verify_funcs[name].push_back(new GlobalVerifyWrapper(verify_func));
    printf("[Config] Registered global verify for '%s'\n", name.c_str());
}

void Config::registerOnConfig(const std::string& name, void (*callback_func)(cJSON*))
{
    m_callback_funcs[name].push_back(new GlobalCallbackWrapper(callback_func));
    printf("[Config] Registered global callback for '%s'\n", name.c_str());
}

std::string Config::toJsonString() 
{
    if (!m_root) return "{}";
    
    char* str = cJSON_Print(m_root);
    std::string result(str);
    free(str);
    return result;
}

bool Config::updateFromJson(const std::string& json_str) 
{
    cJSON* new_config = cJSON_Parse(json_str.c_str());
    if (!new_config) {
        printf("[Config] Invalid JSON string\n");
        return false;
    }
    
    // 遍历新配置，逐个更新
    cJSON* item = new_config->child;
    bool all_success = true;
    
    while (item) 
    {
        if (!setConfig(item->string, item)) 
        {
            printf("[Config] Failed to update '%s'\n", item->string);
            all_success = false;
        }
        item = item->next;
    }
    
    cJSON_Delete(new_config);
    
    if (all_success) 
    {
        save();
    }
    
    return all_success;
}