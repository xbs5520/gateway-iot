#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>
#include "../../external/json.hpp"
#include <vector>
#include <map>
#include <shared_mutex>
#include <thread>
#include <atomic>
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
    bool removeVerify(const std::string& name, bool (*verify_func)(const json&));

    // 注册成员函数
    template<typename T>
    void registerVerify(const std::string& name, T* obj, bool (T::*verify_func)(const json&));
    template<typename T>
    bool removeVerify(const std::string& name, T* obj, bool (T::*verify_func)(const json&));

    // ========== 注册更新回调 ==========
    
    // 注册全局函数或静态函数
    void registerOnConfig(const std::string& name, void (*callback_func)(const json&));
    bool removeOnConfig(const std::string& name, void (*callback_func)(const json&));

    // 注册成员函数
    template<typename T>
    void registerOnConfig(const std::string& name, T* obj, void (T::*callback_func)(const json&));
    template<typename T>
    bool removeOnConfig(const std::string& name, T* obj, void (T::*callback_func)(const json&));

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
    // force save
    bool saveIfDirty();
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
        virtual void* getOwner() const = 0;
    };
    
    struct CallbackWrapper 
    {
        virtual ~CallbackWrapper() = default;
        virtual void call(const json& config) = 0;
        virtual void* getOwner() const = 0;
    };
    
    // global wrapper
    struct GlobalVerifyWrapper : public VerifyWrapper 
    {
        bool (*func)(const json&);
        GlobalVerifyWrapper(bool (*f)(const json&)) : func(f) {}
        bool call(const json& config) override { return func(config); }
        void* getOwner() const override { return nullptr; }
    };
    struct GlobalCallbackWrapper : public CallbackWrapper 
    {
        void (*func)(const json&);
        GlobalCallbackWrapper(void (*f)(const json&)) : func(f) {}
        void call(const json& config) override { func(config); }
        void* getOwner() const override { return nullptr; }
    };
    
    // class wrapper
    template<typename T>
    struct MemberVerifyWrapper : public VerifyWrapper 
    {
        T* obj;
        bool (T::*func)(const json&);
        MemberVerifyWrapper(T* o, bool (T::*f)(const json&)) : obj(o), func(f) {}
        bool call(const json& config) override { return (obj->*func)(config); }
        void* getOwner() const override { return static_cast<void*>(obj); }
    };

    template<typename T>
    struct MemberCallbackWrapper : public CallbackWrapper 
    {
        T* obj;
        void (T::*func)(const json&);
        MemberCallbackWrapper(T* o, void (T::*f)(const json&)) : obj(o), func(f) {}
        void call(const json& config) override { (obj->*func)(config); }
        void* getOwner() const override { return static_cast<void*>(obj); }
    };
    
    json m_root = json::object();
    std::string m_file_path;
    std::shared_mutex m_mutex;

    // save flag
    bool m_dirty = false;
    std::atomic<bool> m_auto_save_running{false};
    std::thread m_auto_save_thread;

    // 自动保存相关 default 30s
    void startAutoSave(int interval_seconds = 30);
    void stopAutoSave();

    std::map<std::string, std::vector<VerifyWrapper*>> m_verify_funcs;
    std::map<std::string, std::vector<CallbackWrapper*>> m_callback_funcs;
}; // Config class

template<typename T>
void Config::registerVerify(const std::string& name, T* obj, bool (T::*verify_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_verify_funcs[name].push_back(new MemberVerifyWrapper<T>(obj, verify_func));
    printf("[Config] Registered member verify for '%s'\n", name.c_str());
}

template<typename T>
bool Config::removeVerify(const std::string& name, T* obj, bool (T::*verify_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    auto it = m_verify_funcs.find(name);
    if (it == m_verify_funcs.end()) {
        return false;
    }
    
    auto& wrappers = it->second;
    size_t old_size = wrappers.size();
    
    wrappers.erase(
        std::remove_if(wrappers.begin(), wrappers.end(),
            [obj, verify_func](VerifyWrapper* w) {
                // find MemberVerifyWrapper<T>
                auto* mw = dynamic_cast<MemberVerifyWrapper<T>*>(w);
                if (mw && mw->obj == obj && mw->func == verify_func) {
                    delete w;
                    return true;
                }
                return false;
            }),
        wrappers.end()
    );
    return wrappers.size() < old_size;
}



template<typename T>
void Config::registerOnConfig(const std::string& name, T* obj, void (T::*callback_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_callback_funcs[name].push_back(new MemberCallbackWrapper<T>(obj, callback_func));
    printf("[Config] Registered member callback for '%s'\n", name.c_str());
}
template<typename T>
bool Config::removeOnConfig(const std::string& name, T* obj, void (T::*callback_func)(const json&))
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    auto it = m_callback_funcs.find(name);
    if (it == m_callback_funcs.end()) {
        return false;
    }
    
    auto& wrappers = it->second;
    size_t old_size = wrappers.size();
    
    wrappers.erase(
        std::remove_if(wrappers.begin(), wrappers.end(),
            [obj, callback_func](CallbackWrapper* w) {
                // find MemberCallbackWrapper<T>
                auto* mw = dynamic_cast<MemberCallbackWrapper<T>*>(w);
                if (mw && mw->obj == obj && mw->func == callback_func) {
                    delete w;
                    return true;
                }
                return false;
            }),
        wrappers.end()
    );
    return wrappers.size() < old_size;
}
#endif //__CONFIG_H