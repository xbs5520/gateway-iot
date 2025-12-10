#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>
#include "../../external/json.hpp"
#include <vector>
#include <map>
#include <shared_mutex>

using json = nlohmann::json;

class Config 
{
public:
    static Config& getInstance();
    
    // 加载配置文件
    bool load(const std::string& file_path);
    
    // 保存配置文件
    bool save();
    
    // 获取配置节点（返回json对象）
    json getConfig(const std::string& name);
    
    // 设置配置节点（会验证 + 比较 + 回调）
    bool setConfig(const std::string& name, const json& config);
    
    // ========== 注册验证函数 ==========
    
    // 注册全局函数或静态函数
    void registerVerify(const std::string& name, bool (*verify_func)(const json&));
    
    // 注册成员函数
    template<typename T>
    void registerVerify(const std::string& name, T* obj, bool (T::*verify_func)(const json&));
    
    // ========== 注册更新回调 ==========
    
    // 注册全局函数或静态函数
    void registerOnConfig(const std::string& name, void (*callback_func)(const json&));
    
    // 注册成员函数
    template<typename T>
    void registerOnConfig(const std::string& name, T* obj, void (T::*callback_func)(const json&));
    
    // 获取整个配置的JSON字符串
    std::string toJsonString();
    
    // 从JSON字符串批量更新配置（用于远程下发）
    bool updateFromJson(const std::string& json_str);
    
    // 获取完整配置的根节点（用于Web API）
    json getRoot() 
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_root; 
    }
    
private:
    Config() = default;
    ~Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    // wrapper basic
    struct VerifyWrapper 
    {
        virtual ~VerifyWrapper() = default;
        virtual bool call(const json& config) = 0;
    };
    
    struct CallbackWrapper 
    {
        virtual ~CallbackWrapper() = default;
        virtual void call(const json& config) = 0;
    };
    
    // global wrapper
    struct GlobalVerifyWrapper : public VerifyWrapper 
    {
        bool (*func)(const json&);
        GlobalVerifyWrapper(bool (*f)(const json&)) : func(f) {}
        bool call(const json& config) override { return func(config); }
    };
    struct GlobalCallbackWrapper : public CallbackWrapper 
    {
        void (*func)(const json&);
        GlobalCallbackWrapper(void (*f)(const json&)) : func(f) {}
        void call(const json& config) override { func(config); }
    };
    
    // class wrapper
    template<typename T>
    struct MemberVerifyWrapper : public VerifyWrapper 
    {
        T* obj;
        bool (T::*func)(const json&);
        // constructor
        MemberVerifyWrapper(T* o, bool (T::*f)(const json&)) : obj(o), func(f) {}
        // call
        bool call(const json& config) override { return (obj->*func)(config); }
    };
    
    template<typename T>
    struct MemberCallbackWrapper : public CallbackWrapper 
    {
        T* obj;
        void (T::*func)(const json&);
        MemberCallbackWrapper(T* o, void (T::*f)(const json&)) : obj(o), func(f) {}
        void call(const json& config) override { (obj->*func)(config); }
    };
    
    json m_root = json::object();
    std::string m_file_path;
    std::shared_mutex m_mutex;

    std::map<std::string, std::vector<VerifyWrapper*>> m_verify_funcs;
    std::map<std::string, std::vector<CallbackWrapper*>> m_callback_funcs;
};

template<typename T>
void Config::registerVerify(const std::string& name, T* obj, bool (T::*verify_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_verify_funcs[name].push_back(new MemberVerifyWrapper<T>(obj, verify_func));
    printf("[Config] Registered member verify for '%s'\n", name.c_str());
}

template<typename T>
void Config::registerOnConfig(const std::string& name, T* obj, void (T::*callback_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_callback_funcs[name].push_back(new MemberCallbackWrapper<T>(obj, callback_func));
    printf("[Config] Registered member callback for '%s'\n", name.c_str());
}

#endif //__CONFIG_H