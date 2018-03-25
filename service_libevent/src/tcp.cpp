#include "../include/mylog.h"
#include "../include/tcp.h"

extern int errno;

int Socket(int family, int type, int protocol,log4cpp::Category* root)
{
    int n;

    if ((n = socket(family, type, protocol)) < 0)
        pCrit("socket error",root);

    return n;
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr,log4cpp::Category* root)
{
    int n;

again:
    if ((n = accept(fd, sa, salenptr)) < 0)
    {
        if ((errno == ECONNABORTED) || (errno == EINTR))
            goto again;
        else
            pCrit("accept error",root);
    }
    return n;
}

int Bind(int fd, const struct sockaddr *sa, socklen_t salen,log4cpp::Category* root)
{
    int n;

    if ((n = bind(fd, sa, salen)) < 0)
        pCrit("bind error",root);

    return n;
}

int Connect(int fd, const struct sockaddr *sa, socklen_t salen,log4cpp::Category* root)
{
    int n;

    if ((n = connect(fd, sa, salen)) < 0)
        pCrit("connect error",root);

    return n;
}

int Listen(int fd, int backlog,log4cpp::Category* root)
{
    int n;

    if ((n = listen(fd, backlog)) < 0)
        pCrit("listen error",root);

    return n;
}

int tcp_server_init(const char *addr, int port, int listen_num,log4cpp::Category* root)
{
    struct sockaddr_in servaddr;
    evutil_socket_t listener;
    unsigned long serv_addr;

    listener = Socket(AF_INET, SOCK_STREAM, 0,root);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, addr, &serv_addr);
    servaddr.sin_addr.s_addr = serv_addr;
    servaddr.sin_port = htons(port);

    evutil_make_listen_socket_reuseable(listener);

    Bind(listener, (struct sockaddr *)&servaddr, sizeof(servaddr),root);

    Listen(listener, listen_num,root);

    evutil_make_socket_nonblocking(listener);

    return listener;
}
