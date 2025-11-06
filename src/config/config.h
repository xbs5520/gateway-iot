#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>
#include <cjson/cJSON.h>
#include <vector>
#include <map>

class Config 
{
public:
    static Config& getInstance();
    
    // 加载配置文件
    bool load(const std::string& file_path);
    
    // 保存配置文件
    bool save();
    
    // 获取配置节点（返回cJSON对象）
    cJSON* getConfig(const std::string& name);
    
    // 设置配置节点（会验证 + 比较 + 回调）
    bool setConfig(const std::string& name, cJSON* config);
    
    // ========== 注册验证函数 ==========
    
    // 注册全局函数或静态函数
    void registerVerify(const std::string& name, bool (*verify_func)(cJSON*));
    
    // 注册成员函数
    template<typename T>
    void registerVerify(const std::string& name, T* obj, bool (T::*verify_func)(cJSON*));
    
    // ========== 注册更新回调 ==========
    
    // 注册全局函数或静态函数
    void registerOnConfig(const std::string& name, void (*callback_func)(cJSON*));
    
    // 注册成员函数
    template<typename T>
    void registerOnConfig(const std::string& name, T* obj, void (T::*callback_func)(cJSON*));
    
    // 获取整个配置的JSON字符串
    std::string toJsonString();
    
    // 从JSON字符串批量更新配置（用于远程下发）
    bool updateFromJson(const std::string& json_str);
    
    // 获取完整配置的根节点（用于Web API）
    cJSON* getRoot() { return m_root; }
    
private:
    Config() = default;
    ~Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    // wrapper basic
    struct VerifyWrapper {
        virtual ~VerifyWrapper() = default;
        virtual bool call(cJSON* config) = 0;
    };
    
    struct CallbackWrapper {
        virtual ~CallbackWrapper() = default;
        virtual void call(cJSON* config) = 0;
    };
    
    // global wrapper
    struct GlobalVerifyWrapper : public VerifyWrapper 
    {
        bool (*func)(cJSON*);
        GlobalVerifyWrapper(bool (*f)(cJSON*)) : func(f) {}
        bool call(cJSON* config) override { return func(config); }
    };
    struct GlobalCallbackWrapper : public CallbackWrapper 
    {
        void (*func)(cJSON*);
        GlobalCallbackWrapper(void (*f)(cJSON*)) : func(f) {}
        void call(cJSON* config) override { func(config); }
    };
    
    // class wrapper
    template<typename T>
    struct MemberVerifyWrapper : public VerifyWrapper 
    {
        T* obj;
        bool (T::*func)(cJSON*);
        // constructor
        MemberVerifyWrapper(T* o, bool (T::*f)(cJSON*)) : obj(o), func(f) {}
        // call
        bool call(cJSON* config) override { return (obj->*func)(config); }
    };
    
    template<typename T>
    struct MemberCallbackWrapper : public CallbackWrapper 
    {
        T* obj;
        void (T::*func)(cJSON*);
        MemberCallbackWrapper(T* o, void (T::*f)(cJSON*)) : obj(o), func(f) {}
        void call(cJSON* config) override { (obj->*func)(config); }
    };
    
    cJSON* m_root = nullptr;
    std::string m_file_path;
    
    std::map<std::string, std::vector<VerifyWrapper*>> m_verify_funcs;
    std::map<std::string, std::vector<CallbackWrapper*>> m_callback_funcs;
};

template<typename T>
void Config::registerVerify(const std::string& name, T* obj, bool (T::*verify_func)(cJSON*))
{
    m_verify_funcs[name].push_back(new MemberVerifyWrapper<T>(obj, verify_func));
    printf("[Config] Registered member verify for '%s'\n", name.c_str());
}

template<typename T>
void Config::registerOnConfig(const std::string& name, T* obj, void (T::*callback_func)(cJSON*))
{
    m_callback_funcs[name].push_back(new MemberCallbackWrapper<T>(obj, callback_func));
    printf("[Config] Registered member callback for '%s'\n", name.c_str());
}

#endif //__CONFIG_H