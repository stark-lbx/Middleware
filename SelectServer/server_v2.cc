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
    //���������׽���
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    //printf("socket created\n");

    //���ö˿ڸ���
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //printf("dup-port set\n");

    //�󶨵�ַ��Ϣ
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY/*htonl(INADDR_ANY)*/;
    Bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    //��������������
    Listen(listenfd, 128);

    this->initFDSet();
    //������Ϣ������
    if (handler) this->handle = handler;
    else this->handle = [this] {sample_handle(); };
  }
  ~Select_Server() {
    Close(listenfd);
    // �ر����пͻ�������
    for (int i = 0; i < fd_arr_len; ++i) {
      if (fd_arr[i] >= 0) {
        Close(fd_arr[i]);
      }
    }
  }

  void start() {
    //�������߳�,����select_loop()
    //new thread:
    //  select_loop();
    std::thread th([this]() {
      select_loop(); });
    th.detach();//����������ں�̨
  }
private:
  void initFDSet() {
    // fd_set allset;//���м��ϱ���
    // fd_set rset;//����������
    // int fd_arr[FD_SETSIZE];//�ļ��������ļ�������
    // int maxi;//����ļ��������������е�����
    // int fd_arr_len;//�ļ�����������ĳ���
    FD_ZERO(&allset);//��� ��������
    FD_SET(listenfd, &allset);// ����������fd ��ӵ� ����������

    // ��ʼ���ļ��������б�
    for (int i = 0; i < FD_SETSIZE; ++i)fd_arr[i] = -1;
    fd_arr[maxi = 0] = listenfd;
    fd_arr_len = 1;
  }
  void select_loop() {
    int retval, readn;
    while (1) {
      rset = allset;//����
      retval = select(fd_arr[maxi] + 1, &rset, NULL, NULL, NULL);
      if (retval < 0)perr_exit("select error");
      //else puts("    server: select begin");

      if (FD_ISSET(listenfd, &rset)) {
        //listenfd�ڼ����У���������� ���¼�
        //puts("    server: fd_isset recv a read_event");

        //����һ�����ӣ���������ͨ�ŵ��ļ�������
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_len = sizeof(clnt_addr);
        int connfd = Accept(listenfd, (struct sockaddr*)&clnt_addr, &clnt_addr_len);

        //��ӵ����� ͨ�� ������������
        FD_SET(connfd, &allset);
        if (connfd > fd_arr[maxi])maxi = fd_arr_len;

        // ����������ӵ�����
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
        //�������������û�и��ļ���������pass
        if (!FD_ISSET(fd_arr[i], &rset))continue;

        //puts("    server: get one,try to readMsg");
        if ((readn = Read(fd_arr[i], buf, sizeof(buf))) == 0) {
          Close(fd_arr[i]);
          FD_CLR(fd_arr[i], &allset);
          fd_arr[i] = -1;
          continue;
        } else {
          //handle(/* buf */);//�ڲ�����buf
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
    //��buf��ȡ����
    //����buf������
    //���޸Ľ�����ص�buf��
    //puts("=====server: sample_handle=====");
    for (int i = 0; i < strlen(buf); ++i) {
      buf[i] = toupper(buf[i]);
    }
  }
private:
  static const int SERV_PORT = 8888;

  int listenfd;//�����׽���
  struct sockaddr_in serv_addr;

  fd_set allset;//���м��ϱ���
  fd_set rset;//����������
  int fd_arr[FD_SETSIZE];//�ļ��������ļ�������
  int fd_arr_len;
  int maxi;

  char buf[BUFSIZ];
  std::function<void(void)> handle;//������Ϣ����
};