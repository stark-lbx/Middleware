// �ͻ��˴���
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
  // ʹ������ָ��������������
  //auto server = std::make_unique<Select_Server>();
  //server->start();
  (new Select_Server())->start();

  // �ȴ���������ʼ�����
  sleep(2);

  // �����ͻ����ļ�������
//�����ͻ����ļ�������
  int clntfd = socket(AF_INET, SOCK_STREAM, 0);
  if (clntfd < 0) {
    perror("client: create socket error");
    exit(-1);
  }

  //�������ӷ�����
  struct sockaddr_in addr; //�������ĵ�ַ�ṹ
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERV_PORT);
  inet_pton(AF_INET, SERV_IP, (void*)&addr.sin_addr.s_addr);
  if (-1 == connect(clntfd, (struct sockaddr*)&addr, sizeof(addr))) {
    perror("client: connect to server error");
    exit(-1);
  }

  //׼��д����
  const char* msg = "hello,socket";
  write(clntfd, msg, strlen(msg));

  int readn;
  const int BUFFER_SIZE = 1024;
  char buf[BUFFER_SIZE];
  memset(buf, 0, sizeof(buf));
  while ((readn = read(clntfd, buf, sizeof(buf))) > 0) {
    //�������Է���������Ӧ
    //puts("client: recv a message from server:");
    puts(buf);
    memset(buf, 0, sizeof(buf));

    //send a request again
    char message[1024];
    scanf("%s", message);
    write(clntfd, message, strlen(message));
    memset(message, 0, sizeof(message));
  }

  close(clntfd); //�رտͻ��˾��
  return 0;
}