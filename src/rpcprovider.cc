#include"rpcprovider.h"
#include"logger.h"
#include"zookeeperutil.h"

//这里是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service* service)
{
    ServiceInfo service_info;

    //获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor* pserverDesc = service->GetDescriptor();
    //获取服务对象的名称
    std::string service_name = pserverDesc->name();
    //获取服务对象的服务方法数量
    int MethodCnt = pserverDesc->method_count();

    //std::cout<<"service_name:"<<service_name<<" method_count:"<<MethodCnt<<std::endl;
    LOG_INFO("serive_name:%s",service_name.c_str());

    for(int i = 0;i<MethodCnt;i++)
    {
        //获取服务对象指定下标的服务方法描述
        const google::protobuf::MethodDescriptor* pmethodDesc = pserverDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name,pmethodDesc});

       // std::cout<<"method_name:"<<method_name<<std::endl;
        LOG_INFO("method_name:%s",method_name.c_str());
    }
    service_info.m_service = service;
    //插入服务对象
    m_serviceMap.insert({service_name,service_info});
}

//启动rpc服务，开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip,port);

    //创建tcpserver对象
    muduo::net::TcpServer server(&m_eventloop,address,"RpcProvider");
    //绑定连接回到和消息读写方法，分离网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

    //设置线程数量
    server.setThreadNum(4);

    //把当前rpc节点上要发布的服务全部注册在zk上，让rpc client可以从zk上发现服务
    //session的timeout默认为30s，zkclient的网络I/O线程1/3的timeout内不发送心跳则丢弃此节点
    ZkClient zkCli;
    zkCli.Start();//链接zkserver

    for(auto &sp : m_serviceMap)
    {
        //service_name
        std::string service_path = "/"+sp.first;//拼接路径
        zkCli.Create(service_path.c_str(),nullptr,0);//创建临时性节点
        for(auto &mp :sp.second.m_methodMap)
        {
            //service_name/method_name
            std::string method_path = service_path+"/"+mp.first;//拼接服务器路径和方法路径
            char method_path_data[128]={0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);//向data中写入路径

            //创建节点,ZOO_EPHEMERAL表示临时节点
            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }

    std::cout<<"RpcProvider start service at ip:"<<ip<<" port:"<<port<<std::endl;

    //启动网络服务
    server.start();
    m_eventloop.loop();
}

//新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if(!conn->connected())
    {
        conn->shutdown();
    }
}

/*
在框架内部需要提前协商好通信使用的protobuf数据类型：比如发送过来的数据类型为：service_name,method_name,args
需要定义proto的message类型，进行数据头的序列化和反序列化，为防止TCP的粘包，需要对各个参数进行参数的长度明确

定义header_size（4字节） + header_str + args_str

已建立连接的用户的读写事件回调，网络上如果有一个远程的rpc服务请求，则onmessge方法就会响应
*/

//读写事件回调
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buffer,muduo::Timestamp)
{
    //网络上接收到的远程rpc调用请求的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    //从字符流中读取前四个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size,4,0);

    //根据header_size读取数据头的原始字符流,反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4,header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size = 0;
    if(rpcHeader.ParseFromString(rpc_header_str))
    {
        //数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();

    }
    else
    {
        //数据头反序列化失败
        std::cout<<"rpc_header_str:"<<rpc_header_str<<" parse error!"<<std::endl;
        return;
    }

    //获取rpc请求参数的原始字符流
    std::string args_str = recv_buf.substr(4+header_size,args_size);

    //打印调试信息
    std::cout<<"======================================"<<std::endl;

    std::cout<<"header_size:"<<header_size<<std::endl;
    std::cout<<"rpc_header_str:"<<rpc_header_str<<std::endl;
    std::cout<<"service_name:"<<service_name<<std::endl;
    std::cout<<"method_name:"<<method_name<<std::endl;
    std::cout<<"args_str:"<<args_str<<std::endl;

    std::cout<<"======================================"<<std::endl;

    //获取service对象和method对象
    auto sit = m_serviceMap.find(service_name);
    if(sit == m_serviceMap.end())
    {
        std::cout<<service_name<<" is not exist!"<<std::endl;
        return;
    }

    auto mit = sit->second.m_methodMap.find(method_name);
    if(mit == sit->second.m_methodMap.end())
    {
        std::cout<<method_name<<" is not exist!"<<std::endl;
        return;
    }

    google::protobuf::Service* service = sit->second.m_service; //获取服务对象
    const google::protobuf::MethodDescriptor* method = mit->second; //获取方法对象

    //生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str))
    {
        std::cout<<"request parse error ,content:"<<args_str<<std::endl;
        return;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();

    //给下面的method方法调用，绑定一个closure回调函数
    google::protobuf::Closure* done = google::protobuf::NewCallback
                <RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>
                (this,&RpcProvider::SendRpcResponse,conn,response);
    




    //在框架上根据远程rpc请求，调用当前rpc服务对象的方法
    service->CallMethod(method,nullptr,request,response,done);
}

//Closure回调函数类型,用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message* response)
{
    std::string respone_str;
    if(response->SerializeToString(&respone_str))
    {
        //序列化成功后，通过网络发送rpc方法执行的结果发送给rpc的调用方
        conn->send(respone_str);
    }
    else
    {
        std::cout<<"response serialize error!"<<std::endl;
    }

    //模拟http的短连接服务，由rpcprovider主动断开
    conn->shutdown();
}


