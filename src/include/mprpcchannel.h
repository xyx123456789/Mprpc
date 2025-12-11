#pragma once

#include<google/protobuf/service.h>
#include<google/protobuf/descriptor.h>
#include<google/protobuf/message.h>
#include"rpcheader.pb.h"
#include"mprpcapplication.h"
#include<string>
#include<errno.h>
#include<unistd.h>

//网络编程部分
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>

class MprpcChannel:public google::protobuf::RpcChannel
{
    //所用通过stub代理对象调用远程rpc方法时，都走到这里了，统一做rpc方法调用的数据序列化和网络发送
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);
};