#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <vector>

#include "Epoll.h"
#include "InetAddress.h"
#include "Socket.h"
#include "util.h"

#define MAX_EVENTS 1024
#define READ_BUFFER 1024

void setnonblocking(int fd) {
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void handleReadEvent(int fd) {
  char buf[READ_BUFFER];
  while (true) {
    bzero(&buf, sizeof(buf));
    ssize_t read_bytes = read(fd, buf, sizeof(buf));
    if (read_bytes > 0) {
      printf("message from client fd %d: %s\n", fd, buf);
      write(fd, buf, sizeof(buf));
    } else if (read_bytes == -1 && errno == EINTR) {
      printf("continue reading");
      continue;
    } else if (read_bytes == -1 &&
               ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
      //   printf("finish reading once, errno: %d\n", errno);
      break;
    } else if (read_bytes == 0) {
      printf("client fd %d disconnected\n", fd);
      close(fd);
      break;
    }
  }
}

int main() {
  Socket *serv_sock = new Socket();
  InetAddress *serv_addr = new InetAddress("127.0.0.1", 8888);
  serv_sock->bind(serv_addr);
  serv_sock->listen();
  Epoll *ep = new Epoll();
  serv_sock->setnonblocking();
  ep->addFd(serv_sock->getFd(), EPOLLIN | EPOLLET);
  while (true) {
    std::vector<epoll_event> events = ep->poll();
    int nfds = events.size();
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == serv_sock->getFd()) {  //新客户端连接
        InetAddress *clnt_addr =
            new InetAddress();  //会发生内存泄露！没有delete
        Socket *clnt_sock = new Socket(
            serv_sock->accept(clnt_addr));  //会发生内存泄露！没有delete
        printf("new client fd %d! IP: %s Port: %d\n", clnt_sock->getFd(),
               inet_ntoa(clnt_addr->addr.sin_addr),
               ntohs(clnt_addr->addr.sin_port));
        clnt_sock->setnonblocking();
        ep->addFd(clnt_sock->getFd(), EPOLLIN | EPOLLET);
      } else if (events[i].events & EPOLLIN) {  //可读事件
        handleReadEvent(events[i].data.fd);
      } else {  //其他事件，之后的版本实现
        printf("something else happened\n");
      }
    }
  }
  delete serv_sock;
  delete serv_addr;
  return 0;
}
