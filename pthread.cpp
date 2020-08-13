#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"

template< typename T >
class threadpool
{
public:
    threadpool( connection_pool *connPool,int thread_number = 8, int max_requests = 10000 );
    ~threadpool();
    bool append( T* request );//插入请求队列

private:
    static void* worker( void* arg );//pthread_create调用
    void run();//worker中用this调用

private:
    int m_thread_number;//线程数
    int m_max_requests;//最大请求数
    pthread_t* m_threads;//描述线程池的数组，创建多个线程用线程数组创建
    std::list< T* > m_workqueue;//请求队列
    locker m_queuelocker;//保护请求队列的互斥锁
    sem m_queuestat;//是否有任务要处理的信号量
    bool m_stop;//是否结束
    connection_pool *m_connPool;  //数据库连接池
};

template< typename T >
threadpool< T >::threadpool( connection_pool *connPool, int thread_number, int max_requests ) : 
        m_thread_number( thread_number ), m_max_requests( max_requests ), 
        m_stop( false ), m_threads( NULL )，m_connPool(connPool)
{
    if( ( thread_number <= 0 ) || ( max_requests <= 0 ) )
    {
        throw std::exception();
    }
	//创建线程池数组
    m_threads = new pthread_t[ m_thread_number ];
    if( ! m_threads )
    {
        throw std::exception();
    }
	//创建线程
    for ( int i = 0; i < thread_number; ++i )
    {
        printf( "create the %dth thread\n", i );
        if( pthread_create( m_threads + i, NULL, worker, this ) != 0 )//成功时返回0
        {
            delete [] m_threads;
            throw std::exception();
        }
        //设置为分离线程，不用回收
        if( pthread_detach( m_threads[i] ) )//pthread_detach()即主线程与子线程分离，子线程结束后，资源自动回收。
        {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template< typename T >
threadpool< T >::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
}

template< typename T >
bool threadpool< T >::append( T* request )
{
    m_queuelocker.lock();
    if ( m_workqueue.size() > m_max_requests )
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back( request );
    m_queuelocker.unlock();
    //通过信号量提醒有任务要处理
    m_queuestat.post();
    return true;
}

template< typename T >
void* threadpool< T >::worker( void* arg )
{
	//将参数转为线程池类，调用run
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

template< typename T >
void threadpool< T >::run()
{
	//主线程不断调用read将请求塞入队列，子线程不停的处理，一次请求对应一个线程处理而不是一个客户端对应一个线程
	//处理完一个就处理下一个
    while ( ! m_stop )
    {
    	//信号量等待post
        m_queuestat.wait();
        //操作队列前加锁
        m_queuelocker.lock();
        if ( m_workqueue.empty() )
        {
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if ( ! request )
        {
            continue;
        }
        //对mysql取&，因为构造函数中是**
        connectionRAII mysqlcon(&request->mysql, m_connPool);//获取连接池，函数结束自动释放
        request->process();//取出请求后调用请求类自身的处理函数
    }
}

#endif