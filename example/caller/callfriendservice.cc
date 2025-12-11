#include<iostream>
#include"mprpcapplication.h"
#include"friend.pb.h"

int main(int argc,char** argv)
{
    //初始化
    MprpcApplication::init(argc,argv);

    //演示调用远程的rpc方法login
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
    //rpc方法的请求参数
    fixbug::GetFriendListRequest request;
    request.set_userid(1000);
    //rpc方法的响应
    fixbug::GetFriendListResponse response;
    //发起rpc方法的调用，同步的rpc调用过程，MprpcChannel::callmethod
    MprpcController controller;
    stub.GetFriendList(&controller,&request,&response,nullptr);

    //一次rpc调用完成，打印结果
    if(controller.Failed())
    {
        std::cout<<"rpc login response failed:"<<controller.ErrorText()<<std::endl;
    }
    else
    {
        if(response.result().errcode() == 0)
        {
            std::cout<<"rpc login response success"<<std::endl;
            for(int i = 0;i<response.friends_size();i++)
            {
                std::cout<<"index"<<(i+1)<<" name:"<<response.friends(i)<<std::endl;
            }
        }
        else
        {
            std::cout<<"rpc login response failed:"<<response.result().errmsg()<<std::endl;
        }
    }
    
    return 0;
}