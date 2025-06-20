// ��װĿ�ģ���server.c��̹����У�ͻ���߼��������������߼��ֿ�������ֱ����תman�ֲᡣ
//    ��wrap.h���������ͨ����س����Զ��庯��ԭ�ͣ�������
//    ��wrap.c���������ͨ����س��� �Զ����������壩
// ������ʽ��ϵͳ���ú�����������ĸ��д������鿴man�ֲᣬ��Listen()��Accept()
// �������ܣ�����ϵͳ���ú��������������
// ��server.c��client.c�е��÷�װ���Զ��庯����
//    ���ϱ��룺
//        server.c �� wrap.c ����server
//        client.c �� wrap.c ���� client

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
