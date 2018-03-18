#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>

#include "sql.h"
#include "http.h"
#include "threadpool.h"
#include "wrap.h"

#define SERV_PORT 8080
#define SERV_IP "192.168.1.110"

char head[MAXLINE];

int main(void)
{

    struct sockaddr_in servaddr, cliaddr[MAXLINE];
    struct threadpool *threadpool;
    socklen_t cliaddr_len;
    pthread_t tid;
    struct epoll_event e_node;
    struct epoll_event e_array[MAXLINE];
    FILE *tmp_in, *tmp_out, *log;
    int lfd, cfd, i, client_num;
    unsigned int timeout;
    unsigned long serv_ip;
    int ret;
    int epfd;
    int opt;
    pthread_msg_http msg;

    threadpool = threadpool_init(100, 10); //线程池初始化

    
    


    opt = 1;

    log = fopen("log", "a+");

    serv_ip = 0;

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERV_IP, &serv_ip);
    servaddr.sin_addr.s_addr = serv_ip;
    servaddr.sin_port = htons(SERV_PORT);

    Bind(lfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    Listen(lfd, 20);
    client_num = 0;

    epfd = epoll_create(MAXLINE);
    e_node.events = EPOLLIN;
    e_node.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &e_node);

    fputs("Web Service Running ...\r\n", stdout);

    while (1)
    {
        ret = epoll_wait(epfd, e_array, MAXLINE, -1);
        if (ret == -1)
        {
            perror("poll error:");
        }
        for (i = 0; i < ret; i++)
        {
            if (e_array[i].data.fd == lfd)
            {
                cfd = Accept(lfd, (struct sockaddr *)&cliaddr[client_num], &cliaddr_len);
                tmp_in = fdopen(cfd, "r");
                tmp_out = fdopen(dup(cfd), "w");
                memset(head, 0, sizeof(head));
                fgets(head, sizeof(head), tmp_in);
                if (strstr(head, "HTTP") != NULL)
                { //判断是不是http请求
                    msg.buf = head;
                    msg.fd = cfd;
                    msg.tmp_in = tmp_in;
                    msg.tmp_out = tmp_out;
                    ret = threadpool_add_job(threadpool, deal_http, (void *)&msg);
                }
                else
                {
                    e_node.data.fd = cfd;
                    e_node.events = EPOLLIN|EPOLLET;
                    
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &e_node);
                    fputs("ok\r\n", tmp_out);
                    client_num++;
                }
            }
            else
            {
                memset(head, 0, sizeof(head));
                if (recv(e_array[i].data.fd, head, sizeof(head), 0))
                {                     //先判断是不是已经断开了连接，如果已经断开了则销毁这个，如果没有则写入来的消息到LOG然后回写
                    fputs(head, log); //这一块可以用另一个线程任务来做
                    write(e_array[i].data.fd, head, sizeof(head));
                    write(e_array[i].data.fd, "OK\r\n", 4);
                    fputs("ok\r\n", log);
                }
                else
                { //销毁之后还得把数组整体前移,且移出这个数组，并取消监听
                    shutdown(e_array[i].data.fd, 2);
                    close(e_array[i].data.fd);
                    e_node.data.fd = e_array[i].data.fd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, e_array[i].data.fd, &e_node);
                    client_num--;
                }
            }
        }
    }
    return 0;
}
