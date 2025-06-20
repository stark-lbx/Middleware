#include "wrap.h"

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>


void perr_exit(const char* msg) {
  perror(msg);
  exit(-1);
}
void perr_continue(const char* msg) {
  perror(msg);
  return;
}

int Accept(int fd, struct sockaddr* sa, socklen_t* salenptr) {
  int n;
again:
  if ((n = accept(fd, sa, salenptr)) < 0) {
    if (errno == ECONNABORTED || errno == EINTR) {
      puts("Accept: 'ECONNABORTED' or 'EINTR'");
      goto again;  // һЩ������ԭ���µ�ʧ��Ӧ����������
    } else perr_exit("accept error");
  }
  return n;
}

int Bind(int fd, const struct sockaddr* sa, socklen_t salen) {
  int n;
  if ((n = bind(fd, sa, salen)) < 0) {
    perr_exit("bind error");
  }
  return n;
}

int Connect(int fd, const struct sockaddr* sa, socklen_t salen) {
  int n;
  if ((n = connect(fd, sa, salen)) < 0) {
    perr_exit("connect error");
  }
  return n;
}

int Listen(int fd, int backlog) {
  int n;
  if ((n = listen(fd, backlog)) < 0) {
    perr_exit("listen error");
  }
  return n;
}

int Socket(int family, int type, int protocol) {
  int sockfd;
  if ((sockfd = socket(family, type, protocol)) < 0) {
    perr_exit("socket error");
  }
  return sockfd;
}

int Close(int fd) {
  int n;
  if ((n = close(fd)) < 0) {
    perr_exit("close error");
  }
  return n;
}

ssize_t Read(int fd, void* ptr, size_t nbytes) {
  ssize_t readn;
again:
  if ((readn = read(fd, ptr, nbytes)) < 0) {
    if (errno == EINTR) goto again;
    else return -1;
  }
  return readn;
}

ssize_t Write(int fd, const void* ptr, size_t nbytes) {
  ssize_t writen;
again:
  if ((writen = write(fd, ptr, nbytes)) < 0) {
    if (errno == EINTR) goto again;
    else return -1;
  }

  return writen;
}

ssize_t Readn(int fd, void* vptr, size_t n) {
  size_t nleft;   // ʣ��δ��ȡ���ֽ���
  ssize_t nread;  // �Ѿ��������ֽ���
  char* ptr;

  ptr = (char*)vptr;
  nleft = n;

  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR) nread = 0;
      else return -1;
    } else if (nread == 0) break;

    nleft -= nread;
    ptr += nread;
  }

  return n - nleft;
}

ssize_t Writen(int fd, const void* vptr, size_t n) {
  size_t nleft;
  ssize_t nwritten;
  char* ptr;

  ptr = (char*)vptr;
  nleft = n;

  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0 && errno == EINTR) nwritten = 0;
      else return -1;
    }

    nleft -= nwritten;
    ptr += nwritten;
  }
}

ssize_t my_read(int fd, char* ptr) {
  static int read_cnt;
  static char* read_ptr;
  static char read_buf[100];

  if (read_cnt <= 0) {
  again:
    if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
      if (errno == EINTR) goto again;
      else return -1;
    } else if (read_cnt == 0) return 0;

    read_ptr = read_buf;
  }
  read_cnt--;
  *ptr = *read_ptr++;

  return 1;
}

ssize_t Readline(int fd, void* vptr, size_t maxlen) {
  ssize_t n, rc;
  char c, * ptr;
  ptr = (char*)vptr;

  for (n = 1; n < maxlen; n++) {
    if ((rc = my_read(fd, &c)) == 1) {
      *ptr++ = c;
      if (c == '\n') break;
    } else if (rc == 0) {
      *ptr = 0;
      return n - 1;
    } else return -1;
  }
  *ptr = 0;

  return n;
}