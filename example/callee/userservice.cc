#include<iostream>
#include<string>
#include"user.pb.h"
#include"mprpcapplication.h"
#include"rpcprovider.h"


/*
UserService原来是一个本地服务，提供了两个进程内的本地方法
Login和GetFriendList
*/
class UserService: public fixbug::UserServiceRpc
{
    bool Login(std::string name,std::string pwd)
    {
        std::cout<<"doing local service : login"<<std::endl;
        std::cout<<"name:"<<name<<"pwd:"<<pwd<<std::endl;
        return true;
    }

    bool Register(uint32_t id,std::string name,std::string pwd)
    {
        std::cout<<"doing local service : Register"<<std::endl;
        std::cout<<"id:"<<id<<"name:"<<name<<"pwd:"<<pwd<<std::endl;
        return true;
    }


    void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegiterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id,name,pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();

    }


    /*
    1.caller => Login(LoginRequest) => muduo => callee
    2.callee => Login(LoginRequest) => 交到下面重写的这个login方法
    */
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        //框架给业务上报了请求参数：LoginRequest，应用程序取出相应的已反序列化的数据来做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        //做本地业务
        bool Login_result = Login(name,pwd);

        //把响应写入 包括错误码，错误消息，返回值
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(Login_result);

         //执行回调操作  执行响应对象数据的序列化和网络发送(由框架来完成)
         done->Run();
    }
};

int main(int argc,char** argv)
{
    //整个程序启动以后，想使用mprpc框架提供的功能，一定要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::init(argc,argv);

    //provider是rpc服务发布对象,负责把本地的服务发布到rpc框架中，让rpc框架来监听网络请求
    RpcProvider provider;
    provider.NotifyService(new UserService());

    //启动一个rpc服务发布节点，开始提供rpc远程网络调用服务
    provider.Run();

    return 0;
}