#pragma once
#include<unordered_map>
#include<string>


//mprpc框架的配置类，负责加载配置文件
class MprpcConfig
{
public:
    //负责加载配置文件
    void LoadConfigFile(const char* config_file);
    //负责获取配置信息
    std::string Load(const std::string& key);
private:
    //存储加载的配置项
    std::unordered_map<std::string ,std::string> m_configMap;
    //去掉字符串前后的空格
    void Trim(std::string& src_buf);
};