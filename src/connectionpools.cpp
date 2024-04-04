#include "connectionpools.hpp"

// 类内定义，类外初始化
// 线程安全的懒汉单例函数接口
ConnectionPools *ConnectionPools::getConnectionPools()
{
    static ConnectionPools conn; // static 由编译器分配和释放
    return &conn;
}

// 从配置文件中加载配置项
bool ConnectionPools::loadConfigFile()
{
	FILE *pf = fopen("/home/lihuimin/Project/Connection-pools/src/mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}

	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);
		if (idx == -1) // 无效的配置项
		{
			continue;
		}

		// password=123456\n
		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeOut")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}

// 连接池构造函数
ConnectionPools::ConnectionPools()
{
    if (!loadConfigFile())
    {
        LOG("load configfile error!")
        return;
    }

    // 创建_initSize的初始连接
    for (int i = 0; i < _initSize; i++)
    {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime(); // 刷新连接起始时间
        _connectionQue.push(p);
        _connectionCnt++;
    }

    // 启动一个新线程，作为connection生产者
    thread produce(std::bind(&ConnectionPools::produceConnectionTask, this));
    produce.detach();

    // 启动一个新的定时线程，扫描超过最大空闲时间的控线连接，进行连接释放操作
    thread scanner(std::bind(&ConnectionPools::scannerConnectionTask, this));
    scanner.detach();
}

// 生产新连接
void ConnectionPools::produceConnectionTask()
{
    for (;;)
    {
        unique_lock<mutex> lock(_queueMutex);
        while (!_connectionQue.empty())
        {
            cv.wait(lock); // 连接池队列不空，生产者线程进入等待状态
        }
        if (_connectionCnt < _maxSize)
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime(); // 刷新连接起始时间
            _connectionQue.push(p);
            _connectionCnt++;
        }

        // 生产完毕后，通知消费者线程可以消费了
        cv.notify_all();
    }
}

// 给外部提供接口，从连接池中拿连接
shared_ptr<Connection> ConnectionPools::getConnection()
{
    unique_lock<mutex> lock(_queueMutex);
    while (_connectionQue.empty())
    {

        // 判断有新的连接产生还是到达获取连接的超时时间，带超时时间的mutex互斥锁来实现连接超时时间
        if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
        {
            if (_connectionQue.empty())
            {
                // 获取空闲连接超时，获取连接失败
                LOG("Get Idle Connection Timed Out, Get Connection Failed!")
                return nullptr;
            }
        }
    }

    // 生产者线程生产出新的连接
    /*使用shared_ptr管理新生产的连接，智能指针析构时，会把connection资源直接释放掉，相当于调用了connection
    的析构函数，connection就关闭了。
    使用匿名表达式自定义shared_ptr的释放资源方式，将connection归还给线程队列即可
    [&]：默认以引用捕获所有变量*/
    shared_ptr<Connection> sp(_connectionQue.front(), [&](Connection *ptr)
                              {
        unique_lock<mutex>lock(_queueMutex);
        ptr->refreshAliveTime();
        _connectionQue.push(ptr); });

    _connectionQue.pop();
    cv.notify_all(); // 消费完连接后，通知生产者线程检查连接池队列状态，如果为空可以生产连接了
    return sp;
}

// 运行在单独的定时线程中，用于扫描空闲连接，进行连接资源回收
void ConnectionPools::scannerConnectionTask()
{
    for (;;)
    {
        // 通过sleep函数模拟定时效果
        this_thread::sleep_for(chrono::seconds(_maxIdleTime));

        // 扫描整个队列，判断是否有超过最大空余时间的空闲连接，保证线程安全加锁
        unique_lock<mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection *ptr = _connectionQue.front();
            if (ptr->getAliveTime() >= (_maxIdleTime * 1000))
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete ptr; // 调用Connection的西沟函数
            }
            else
            {
                break; // 如果队头都不超过最大空余时间，则对头后续连接肯定也不超过
            }
        }
    }
}
