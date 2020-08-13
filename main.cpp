  //创建MAX_FD个http类对象
  http_conn* users=new http_conn[MAX_FD];
  
  //创建内核事件表
  epoll_event events[MAX_EVENT_NUMBER];
  epollfd = epoll_create( );
  assert(epollfd != - );
  
  //将listenfd放在epoll树上
  addfd(epollfd, listenfd, false);
  
  //将上述epollfd赋值给http类对象的m_epollfd属性
  http_conn::m_epollfd = epollfd;//？？？
  
  while (!stop_server)
  {
      //等待所监控文件描述符上有事件的产生
      int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, - );
      if (number < 0  && errno != EINTR)
      {
          break;
      }
      //对所有就绪事件进行处理
      for (int i = 0; i < number; i++)
      {
          int sockfd = events[i].data.fd;
  
          //处理新到的客户连接
          if (sockfd == listenfd)
          {
              struct sockaddr_in client_address;
              socklen_t client_addrlength = sizeof(client_address);
  //LT水平触发
  #ifdef LT
              int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
              if (connfd < 0)
              {
                  continue;
              }
              if (http_conn::m_user_count >= MAX_FD)
              {
                  show_error(connfd, "Internal server busy");
                  continue;
              }
              users[connfd].init(connfd, client_address);
  #endif
  
  //ET非阻塞边缘触发
  #ifdef ET
              //需要循环接收数据
              while (1)
              {
                  int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                  if (connfd < 0)
                  {
                      break;
                  }
                  if (http_conn::m_user_count >= MAX_FD)
                  {
                      show_error(connfd, "Internal server busy");
                      break;
                  }
                  users[connfd].init(connfd, client_address);
              }
              continue;
  #endif
          }
  
          //处理异常事件
          else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
          {
              //服务器端关闭连接
          }
  
          //处理信号
          else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN))
          {
          }
  
          //处理客户连接上接收到的数据
          else if (events[i].events & EPOLLIN)
          {
              //读入对应缓冲区
              if (users[sockfd].read_once())
              {
                  //若监测到读事件，将该事件放入请求队列
                  pool->append(users + sockfd);
              }
              else
              {
                 //服务器关闭连接
              }
          }
  
      }
  }