// 客户端代码
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <memory>
#include "server_v2.cc"

#define SERV_IP "127.0.0.1"
#define SERV_PORT 8888

int main() {
  // 使用智能指针管理服务器对象
  //auto server = std::make_unique<Select_Server>();
  //server->start();
  (new Select_Server())->start();

  // 等待服务器初始化完成
  sleep(2);

  // 创建客户端文件描述符
//创建客户端文件描述符
  int clntfd = socket(AF_INET, SOCK_STREAM, 0);
  if (clntfd < 0) {
    perror("client: create socket error");
    exit(-1);
  }

  //尝试连接服务器
  struct sockaddr_in addr; //服务器的地址结构
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERV_PORT);
  inet_pton(AF_INET, SERV_IP, (void*)&addr.sin_addr.s_addr);
  if (-1 == connect(clntfd, (struct sockaddr*)&addr, sizeof(addr))) {
    perror("client: connect to server error");
    exit(-1);
  }

  //准备写东西
  const char* msg = "hello,socket";
  write(clntfd, msg, strlen(msg));

  int readn;
  const int BUFFER_SIZE = 1024;
  char buf[BUFFER_SIZE];
  memset(buf, 0, sizeof(buf));
  while ((readn = read(clntfd, buf, sizeof(buf))) > 0) {
    //处理来自服务器的响应
    //puts("client: recv a message from server:");
    puts(buf);
    memset(buf, 0, sizeof(buf));

    //send a request again
    char message[1024];
    scanf("%s", message);
    write(clntfd, message, strlen(message));
    memset(message, 0, sizeof(message));
  }

  close(clntfd); //关闭客户端句柄
  return 0;
}