#ifndef __TCP_H_
#define __TCP_H_

#include <sys/types.h>          
#include <sys/socket.h>
#include <event.h>
#include <arpa/inet.h>
#include <iostream>

#include "../include/mylog.h"

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr,log4cpp::Category* root);
int Bind(int fd, const struct sockaddr *sa, socklen_t salen,log4cpp::Category* root);
int Connect(int fd, const struct sockaddr *sa, socklen_t salen,log4cpp::Category* root);
int Listen(int fd, int backlog,log4cpp::Category* root);
int Socket(int family, int type, int protocol,log4cpp::Category* root);
int tcp_server_init(const char *addr,int port,int listen_num,log4cpp::Category* root);

#endif