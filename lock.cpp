#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

class sem{
public:
    /*创建并初始化信号量*/
    sem(){
        if(sem_init(&m_sem, 0, 0)!= 0)  //信号量初始化为0
            throw exception();
    }
    /*销毁信号量*/
    ~sem(){
        sem_destroy(&m_sem);
    }
    /*等待信号量变为非0，然后-1*/
    bool wait(){
        return sem_wait(&m_sem) == 0;
    }
    /*增加信号量，+1*/
    bool post(){
        return sem_post(&m_sem) == 0;
    }
private:
    sem_t m_sem;
};

/*封装互斥锁*/
class locker{
public:
    /*创建并初始化锁*/
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL) != 0)
            throw exception();
    }
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }
    /*加锁*/
    bool lock(){
        return pthread_mutex_lock(&m_mutex);
    }
    /*解锁*/
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex);
    }
    pthread_mutex_t* get(){
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

/*封装条件变量*/
class cond{
public:
    cond(){
        /*if(pthread_mutex_init(&m_mutex,NULL) != 0)
            throw exception();*/
        if(pthread_cond_init(&m_cond,NULL) != 0){
            //pthread_mutex_destroy(&m_mutex);//条件变量创建失败，释放资源
            throw exception();
        }
    }
    ~cond(){
        //pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    /*等待条件变量:使线程挂起，等待某个条件变量的信号，注意前后要加解锁*/
    bool wait(){
        int ret=0;
        //pthread_mutex_lock(&m_mutex);
        ret=pthread_cond_wait(&m_cond,&m_mutex);
        //pthread_mutex_lock(&m_mutex);
        return ret==0;
    }
    /*等待一定的时间，指定时间内等待信号量*/
    bool timewait(pthread_mutex_t *m_mutex,struct timespec t){
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret=pthread_cond_timedwait(&m_cond,m_mutex,&t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    /*唤醒等待条件变量的线程*/
    bool signal(){
        return pthread_cond_signal(&m_cond) == 0;
    }
    /*唤醒多个等待条件变量的线程*/
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    //pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    
};
#endif