#include <iostream>
using namespace std;
#include "connection.hpp"
#include "connectionpools.hpp"

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "connection";
static int port = 3306;

//普通MySQL连接压力测试
void SimpleConnect(int count);
//连接池压力测试
void ConnectPools(int count);

//多线程普通MySQL连接池
void thread_SC(int count){
    for (int i = 0; i < count; i++)
    {
        Connection conn;
        char sql[1024] = {0};
        sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')", "zhang san", 20, "male");
        conn.connect(server, port, user, password, dbname);
        conn.update(sql);
    }
}

//多线程连接池
void thread_CP(int count){
    ConnectionPools *cp = ConnectionPools::getConnectionPools();
	for (int i = 0; i < count; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
			"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
        }
}



int main()
{
    clock_t begin = clock();
    int count = 1000;   
    int t_count = count/4;
    Connection conn;
    conn.connect(server, port, user, password, dbname);
    thread t1([&](){
        thread_SC(t_count);
});
    thread t2([&](){
        thread_SC(t_count);
});
    thread t3([&](){
        thread_SC(t_count);

});
    thread t4([&](){
        thread_SC(t_count);
});

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    clock_t end = clock();
    cout << "多线程MySQL连接池压力测试：" << count << "次耗费时间为：" << end - begin << "ms" << endl;

    return 0;
}
//普通MySQL连接压力测试
void SimpleConnect(int count)
{
    clock_t begin = clock();
    for (int i = 0; i < count; i++)
    {
        Connection conn;
        char sql[1024] = {0};
        sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')", "zhang san", 20, "male");
        conn.connect(server, port, user, password, dbname);
        conn.update(sql);
    }
    clock_t end = clock();
    cout << "普通MySQL连接压力测试:" << count << "次耗费时间为：" << end - begin << "ms" << endl;
}
//连接池压力测试
void ConnectPools(int count)
{
    clock_t begin = clock();
    ConnectionPools *cp = ConnectionPools::getConnectionPools();
	for (int i = 0; i < count; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
        }
    clock_t end = clock();
    cout << "连接池压力测试：" << count << "次耗费时间为：" << end - begin << "ms" << endl;
}
