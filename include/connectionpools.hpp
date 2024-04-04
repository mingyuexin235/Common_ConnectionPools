#ifndef CONNECTIONPOOLS_H
#define CONNECTIONPOOLS_H

#include <iostream>
using namespace std;
#include <queue>
#include "connection.hpp"
#include <mutex>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>

/*实现连接池功能模块*/

class ConnectionPools
{
public:
    //对外开放连接池接口
    static ConnectionPools *getConnectionPools();

    //运行在单独的线程中，用于生产连接
    void produceConnectionTask();

    //给外部提供接口，从连接池中拿连接
    shared_ptr<Connection> getConnection();

    //运行在单独的定时线程中，用于扫描空闲连接，进行连接资源回收
    void scannerConnectionTask();


private:
    ConnectionPools(); // 构造函数私有化

    bool loadConfigFile();   //从配置文件中加载配置项

    string _ip;           // IP地址
    unsigned short _port; // 端口号
    string _username;     // MySQL登录用户名
    string _password;     // MySQL登录密码
    string _dbname;       // MySQL数据库名称

    int _initSize;          // 初始连接量
    int _maxSize;           // 最大连接量
    int _maxIdleTime;       // 最大空余时间
    int _connectionTimeout; // 获取连接的超时时间

    queue<Connection *> _connectionQue; // 存储MySQL连接的队列
    mutex _queueMutex;                  // 维护队列线程安全的互斥锁
    //原子类型保证线程安全  
    atomic_int _connectionCnt;          //记录所创建的connection总量
    condition_variable cv;              //条件变量，用于生产者和消费者线程间通信


};

#endif