#include "config.h"
#include <fstream>

Config& Config::getInstance() 
{
    static Config instance;
    return instance;
}

Config::~Config() 
{
    // clear warpper
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
    std::ifstream file(file_path);
    if (!file.is_open()) 
    {
        printf("[Config] File not found: %s, creating empty config\n", file_path.c_str());
        m_root = json::object();
        return false;
    }
    
    json temp_root;
    try {
        file >> temp_root;
    }
    catch (json::exception& e) {
        printf("[Config] Parse error: %s\n", e.what());
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_file_path = file_path;
        m_root = json::object();
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_file_path = file_path;
    m_root = std::move(temp_root);
    printf("[Config] Loaded from %s\n", file_path.c_str());
    return true;
}

bool Config::save() 
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    std::ofstream file(m_file_path);
    if (!file.is_open()) {
        printf("[Config] Failed to save to %s\n", m_file_path.c_str());
        return false;
    }
    
    file << m_root.dump(4);
    printf("[Config] Saved to %s\n", m_file_path.c_str());
    return true;
}

json Config::getConfig(const std::string& name) 
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (m_root.contains(name)) {
        return m_root[name];
    }
    return json::object();
}

bool Config::setConfig(const std::string& name, const json& config) 
{
    std::vector<VerifyWrapper*> verify_list;
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        // 0. check config is changed ?
        if (m_root.contains(name) && m_root[name] == config) 
        {
            printf("[Config] '%s' not changed, skip update\n", name.c_str());
            return true; 
        }
        
        // copy list
        auto verify_it = m_verify_funcs.find(name);
        if (verify_it != m_verify_funcs.end()) 
        {
            verify_list = verify_it->second;
        }
    }
    // 1. all verify
    for (auto wrapper : verify_list) 
    {
        if (!wrapper->call(config)) 
        {
            printf("[Config] Verification failed for '%s'\n", name.c_str());
            return false;
        }
    }
    
    std::vector<CallbackWrapper*> callback_list;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        //2. update config
        m_root[name] = config;
        
        // copy list
        auto callback_it = m_callback_funcs.find(name);
        if (callback_it != m_callback_funcs.end()) 
        {
            callback_list = callback_it->second;
        }
    }
    
    // 3. all call back
    for (auto wrapper : callback_list) 
    {
        wrapper->call(config);
    }
    
    printf("[Config] Updated '%s'\n", name.c_str());
    return true;
}

void Config::registerVerify(const std::string& name, bool (*verify_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_verify_funcs[name].push_back(new GlobalVerifyWrapper(verify_func));
    printf("[Config] Registered global verify for '%s'\n", name.c_str());
}

void Config::registerOnConfig(const std::string& name, void (*callback_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_callback_funcs[name].push_back(new GlobalCallbackWrapper(callback_func));
    printf("[Config] Registered global callback for '%s'\n", name.c_str());
}

std::string Config::toJsonString() 
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_root.dump(4);
}

bool Config::updateFromJson(const std::string& json_str) 
{
    try {
        json new_config = json::parse(json_str);
        
        bool all_success = true;
        
        for (auto& [key, value] : new_config.items()) {
            if (!setConfig(key, value)) {
                printf("[Config] Failed to update '%s'\n", key.c_str());
                all_success = false;
            }
        }
        
        if (all_success) {
            save();
        }
        
        return all_success;
    }
    catch (json::exception& e) {
        printf("[Config] Invalid JSON string: %s\n", e.what());
        return false;
    }
}