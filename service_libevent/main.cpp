#include <iostream>
#include <string>
#include <event.h>

#include "./include/mylog.h"
#include "./include/tcp.h"

void socket_read_cb(bufferevent *bev, void *arg)
{
    char msg[4096];

    size_t len = bufferevent_read(bev, msg, sizeof(msg));

    msg[len] = '\0';
    printf("recv the client msg: %s", msg);

    char reply_msg[4096] = "I have recvieced the msg: ";

    strcat(reply_msg + strlen(reply_msg), msg);
    bufferevent_write(bev, reply_msg, strlen(reply_msg));
}

void event_cb(struct bufferevent *bev, short event, void *arg)
{

    if (event & BEV_EVENT_EOF)
        printf("connection closed\n");
    else if (event & BEV_EVENT_ERROR)
        printf("some other error\n");

    //这将自动close套接字和free读写缓冲区
    bufferevent_free(bev);
}

void accept_cb(int fd, short events, void *arg)
{
    evutil_socket_t sockfd;

    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    sockfd = ::accept(fd, (struct sockaddr *)&client, &len);
    evutil_make_socket_nonblocking(sockfd);

    printf("accept a client %d\n", sockfd);

    struct event_base *base = (event_base *)arg;

    bufferevent *bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, socket_read_cb, NULL, event_cb, arg);

    bufferevent_enable(bev, EV_READ | EV_PERSIST);
}

int main(int argc, char **argv)
{
    std::string addr;
    int port;
    struct event_base *base;
    struct event *e_listen;
    int lfd;
    log4cpp::Category* root;

    root = mylog_init();

    if (argc < 3)
    {
        pError("main error missing argument",root);
        std::cout << "Service [error]: missing argument" << std::endl;
        std::cout<<"service [addr] [port]"<<std::endl;
        return -1;
    }
    if (argc > 3)
    {
        pError("main error more argument",root);
        std::cout << "Serviece [error]:more argument" << std::endl;
        std::cout<<"service [addr] [port]"<<std::endl;
        return -1;
    }

    addr = argv[1];
    port = atoi(argv[2]);

    lfd = tcp_server_init(addr.c_str(), port, 10,root);

    base = event_base_new();

    e_listen = event_new(base, lfd, EV_READ | EV_PERSIST, accept_cb, base);

    event_add(e_listen, NULL);

    event_base_dispatch(base);

    event_base_free(base);

    return 0;
}
