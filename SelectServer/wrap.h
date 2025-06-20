// 封装目的：在server.c编程过程中，突出逻辑，将出错处理与逻辑分开，可以直接跳转man手册。
//    【wrap.h】存放网络通信相关常用自定义函数原型（声明）
//    【wrap.c】存放网络通信相关常用 自定函数（定义）
// 命名方式：系统调用函数名的首字母大写，方便查看man手册，如Listen()、Accept()
// 函数功能：调用系统调用函数，处理出错场景
// 在server.c和client.c中调用封装的自定义函数。
//    联合编译：
//        server.c 和 wrap.c 生成server
//        client.c 和 wrap.c 生成 client

// ====================================================================
#ifndef __SOCKET_WRAP_H_STARK__
#define __SOCKET_WRAP_H_STARK__

#include <arpa/inet.h>

void perr_exit(const char *msg);
void perr_continue(const char *msg);

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Listen(int fd, int backlog);
int Socket(int family, int type, int protocol);
int Close(int fd);

ssize_t Read(int fd, void *ptr, size_t nbytes);
ssize_t Write(int fd, const void *ptr, size_t nbytes);

ssize_t Readn(int fd, void *vptr, size_t n);
ssize_t Writen(int fd, const void *vptr, size_t n);

ssize_t my_read(int fd, char *ptr);
ssize_t Readline(int fd, void *vptr, size_t maxlen);

#endif  // !__SOCKET_WRAP_H_STARK__
