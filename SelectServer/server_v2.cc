#include "wrap.h"
#include <cstring>
#include <cstdio>
#include <functional>
#include <ctype.h>
#include <thread>
#include <unistd.h>
class Select_Server {
public:
  Select_Server(std::function<void(void)> handler = nullptr) {
    //创建监听套接字
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    //printf("socket created\n");

    //设置端口复用
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //printf("dup-port set\n");

    //绑定地址信息
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY/*htonl(INADDR_ANY)*/;
    Bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    //设置最大监听数量
    Listen(listenfd, 128);

    this->initFDSet();
    //设置消息处理函数
    if (handler) this->handle = handler;
    else this->handle = [this] {sample_handle(); };
  }
  ~Select_Server() {
    Close(listenfd);
    // 关闭所有客户端连接
    for (int i = 0; i < fd_arr_len; ++i) {
      if (fd_arr[i] >= 0) {
        Close(fd_arr[i]);
      }
    }
  }

  void start() {
    //开启子线程,运行select_loop()
    //new thread:
    //  select_loop();
    std::thread th([this]() {
      select_loop(); });
    th.detach();//服务端运行在后台
  }
private:
  void initFDSet() {
    // fd_set allset;//所有集合备份
    // fd_set rset;//监听读集合
    // int fd_arr[FD_SETSIZE];//文件描述符的监听数组
    // int maxi;//最大文件描述符在数组中的索引
    // int fd_arr_len;//文件描述符数组的长度
    FD_ZERO(&allset);//清空 监听集合
    FD_SET(listenfd, &allset);// 将待监听的fd 添加到 监听集合中

    // 初始化文件描述符列表
    for (int i = 0; i < FD_SETSIZE; ++i)fd_arr[i] = -1;
    fd_arr[maxi = 0] = listenfd;
    fd_arr_len = 1;
  }
  void select_loop() {
    int retval, readn;
    while (1) {
      rset = allset;//备份
      retval = select(fd_arr[maxi] + 1, &rset, NULL, NULL, NULL);
      if (retval < 0)perr_exit("select error");
      //else puts("    server: select begin");

      if (FD_ISSET(listenfd, &rset)) {
        //listenfd在集合中，满足监听的 读事件
        //puts("    server: fd_isset recv a read_event");

        //建立一个连接，返回用于通信的文件描述符
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_len = sizeof(clnt_addr);
        int connfd = Accept(listenfd, (struct sockaddr*)&clnt_addr, &clnt_addr_len);

        //添加到监听 通信 描述符集合中
        FD_SET(connfd, &allset);
        if (connfd > fd_arr[maxi])maxi = fd_arr_len;

        // 将新连接添加到数组
        for (int i = 0; i < FD_SETSIZE; ++i) {
          if (fd_arr[i] == -1) {
            fd_arr[i] = connfd;
            if (i >= fd_arr_len)
              fd_arr_len = i + 1;
            break;
          }
        }//fd_arr[fd_arr_len++] = connfd;


        if (retval == 1)continue;
      }

      //puts("    server: find the read_event's fd");
      for (int i = 0; i < fd_arr_len; ++i) {
        //如果监听集合中没有该文件描述符，pass
        if (!FD_ISSET(fd_arr[i], &rset))continue;

        //puts("    server: get one,try to readMsg");
        if ((readn = Read(fd_arr[i], buf, sizeof(buf))) == 0) {
          Close(fd_arr[i]);
          FD_CLR(fd_arr[i], &allset);
          fd_arr[i] = -1;
          continue;
        } else {
          //handle(/* buf */);//内部设置buf
          sample_handle();
          //puts("    server: send to respond");
          Write(fd_arr[i], buf, strlen(buf));
          memset(buf, 0, sizeof(buf));
          //puts("    server: send done");
        }
      }
    }
  }

  void sample_handle() {
    //从buf中取数据
    //处理buf中数据
    //将修改结果返回到buf中
    //puts("=====server: sample_handle=====");
    for (int i = 0; i < strlen(buf); ++i) {
      buf[i] = toupper(buf[i]);
    }
  }
private:
  static const int SERV_PORT = 8888;

  int listenfd;//监听套接字
  struct sockaddr_in serv_addr;

  fd_set allset;//所有集合备份
  fd_set rset;//监听读集合
  int fd_arr[FD_SETSIZE];//文件描述符的监听数组
  int fd_arr_len;
  int maxi;

  char buf[BUFSIZ];
  std::function<void(void)> handle;//处理消息函数
};