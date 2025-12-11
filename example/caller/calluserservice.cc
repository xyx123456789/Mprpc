#include<iostream>
#include"mprpcapplication.h"
#include"user.pb.h"
#include"mprpcchannel.h"

int main(int argc,char** argv)
{

    

    //初始化
    MprpcApplication::init(argc,argv);

    //演示调用远程的rpc方法login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    //rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    //rpc方法的响应
    fixbug::LoginResponse response;
    //发起rpc方法的调用，同步的rpc调用过程，MprpcChannel::callmethod
    stub.Login(nullptr,&request,&response,nullptr);

    //一次rpc调用完成，打印结果
    if(response.result().errcode() == 0)
    {
        std::cout<<"rpc login response success:"<<response.success()<<std::endl;
    }
    else
    {
        std::cout<<"rpc login response failed:"<<response.result().errmsg()<<std::endl;
    }

    //演示调用远程的rpc方法register
    fixbug::RegiterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");
    fixbug::RegisterResponse rsp;

    stub.Register(nullptr,&req,&rsp,nullptr);

    if(rsp.result().errcode() == 0)
    {
        std::cout<<"rpc Register response success:"<<rsp.success()<<std::endl;
    }
    else
    {
        std::cout<<"rpc Register response failed:"<<rsp.result().errmsg()<<std::endl;
    }
    return 0;
}