#include"logger.h"
#include<time.h>
#include<iostream>

Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    //启动专门的写日志线程
    std::thread writeLogTask([&](){
        for(;;)
        {
            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char file_name[128] = {0};
            sprintf(file_name, "%04d-%02d-%02d-log.txt", nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday);

            FILE *fp = fopen(file_name, "a+");
            if(fp==nullptr)
            {
                std::cout<<"log file:"<<file_name<<"open log file error!"<<std::endl;
                exit(EXIT_FAILURE);
            }

            std::string msg = m_lckQue.Pop();

            char time_buf[128] = {0};
            sprintf(time_buf,"%d:%d:%d =>[%s]", 
                        nowtm->tm_hour, 
                        nowtm->tm_min, 
                        nowtm->tm_sec,
                        (m_loglevel==INFO?"info":"error"));
            msg.insert(0,time_buf);
            msg.append("\n");

            fputs(msg.c_str(),fp);
            fclose(fp);
        }
    });

    writeLogTask.detach();
}


//设置日志级别
void Logger::SetLogLevel(Loglevel level)
{
    m_loglevel = level;
}

//写日志
void Logger::Log(std::string msg)
{
    m_lckQue.Push(msg);
}