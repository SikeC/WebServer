#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>

#include "wrap.h"

#define MAXLINE 1024
#define SERV_PORT 8888
#define SERV_IP "192.168.1.107"

char head[MAXLINE];

int fdin;
int fdout;

typedef struct pthread_msg_http
{
    char *buf;
    int fd;
} pthread_msg_http;

void http_send_headers(char *type, off_t length)
{
    printf("%s %d %s\r\n", "HTTP/1.1", 200, "0k");
    printf("Content-Type:%s\r\n", type);
    printf("Content-Length:%ld\r\n", (int64_t)length);
    printf("Connection: close\r\n");
    printf("\r\n");
}

void http_send_err(int status, char *title, char *text)
{
    http_send_headers("text/html", -1);

    printf("<html><head><title>%d %s</title></head>\n", status, title);
    printf("<body bgcolor=\"#cc99cc\"><h4>%d %s</h4>\n", status, title);
    printf("%s\n", text);
    printf("<hr>\n</body>\n</html>\n");
    fflush(stdout);
}

void *deal_http(void *arg)
{
    char buf[MAXLINE * 4];
    char method[MAXLINE], path[MAXLINE], protocol[MAXLINE];
    struct stat s_buf;
    char *type;
    pthread_msg_http *msg;
    char *filesname;
    char *dot;
    FILE *fp;
    char ich;

    msg = (pthread_msg_http *)arg;

    dup2(msg->fd, STDIN_FILENO);
    dup2(msg->fd, STDOUT_FILENO);

    if (sscanf(msg->buf, "%[^ ] %[^ ] %[^ ]", method, path, protocol) != 3)
    {
        http_send_err(400, "Bad Request", "Can't parse request.");
        dup2(STDIN_FILENO, fdin);
        dup2(STDOUT_FILENO, fdout);
        goto end;
    }
    while (fgets(buf, sizeof(buf), stdin) != NULL)
    {
        if (strcmp(buf, "\n") == 0 || strcmp(buf, "\r\n") == 0)
        {
            break;
        }
    }

    if (strcasecmp("GET", method) != 0)
    {
        http_send_err(501, "Not Implemented", "That method is not implemented.");
        goto end;
    }
    if (path[0] != '/')
    {
        http_send_err(400, "Bad Request", "Bad filename.");
        goto end;
    }
    filesname = path + 1;
    if (stat(filesname, &s_buf) < 0)
    {
        http_send_err(404, "Not Found", "File not found.");
        goto end;
    }

    fp = fopen(filesname, "r");
    if (fp == NULL)
    {
        http_send_err(403, "Forbidden", "File is protected.");
        fclose(fp);
        goto end;
    }

    dot = strrchr(filesname, '.');

    if (dot == NULL)
    {
        type = "text/plain; charset=utf-8";
    }
    else if (strcmp(dot, ".html") == 0)
    {
        type = "text/html; charset=utf-8";
    }
    else if (strcmp(dot, ".jpg") == 0)
    {
        type = "image/jpeg";
    }
    else if (strcmp(dot, ".gif") == 0)
    {
        type = "image/gif";
    }
    else if (strcmp(dot, ".png") == 0)
    {
        type = "image/png";
    }
    else if (strcmp(dot, ".mp3") == 0)
    {
        type = "audio/mpeg";
    }
    else
    {
        type = "text/plain; charset=iso-8859-1";
    }

    http_send_headers(type, s_buf.st_size);

    while ((ich = getc(fp)) != EOF)
        putchar(ich);

    fflush(stdout);

    fclose(fp);
end:
    dup2(STDIN_FILENO, fdin);
    dup2(STDOUT_FILENO, fdout);
    shutdown(msg->fd, 2);

    return NULL;
}

int main(void)
{

    struct sockaddr_in servaddr, cliaddr[MAXLINE];
    socklen_t cliaddr_len;
    pthread_t tid;
    int lfd, cfd, i, client_num, tmp_num;
    unsigned long serv_ip;
    int ret;
    int log;
    struct pollfd fds[MAXLINE];
    pthread_msg_http msg;

    fdin = dup(STDIN_FILENO);
    fdout = dup(STDOUT_FILENO);

    log = open("log", O_CREAT | O_RDWR);

    serv_ip = 0;

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERV_IP, &serv_ip);
    servaddr.sin_addr.s_addr = serv_ip;
    servaddr.sin_port = htons(SERV_PORT);

    Bind(lfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    Listen(lfd, 20);

    client_num = 0;

    for (i = 0; i < MAXLINE; i++)
    {
        fds[i].fd = -1;
    }

    fds[0].fd = lfd;
    fds[0].events = POLLIN;

    fputs("Web Service Running ...\r\n", stdout);

    while (1)
    {
        ret = poll(fds, client_num + 1, -1);
        if (ret == -1)
        {
            perror("poll error:");
        }
        if (fds[0].revents == POLLIN)
        { //判断是不是新连接的创建
            cfd = Accept(lfd, (struct sockaddr *)&cliaddr[client_num], &cliaddr_len);
            dup2(cfd, STDIN_FILENO);
            memset(head, 0, sizeof(head));
            fgets(head, sizeof(head), stdin);
            dup2(STDIN_FILENO, fdin);
            if (strstr(head, "HTTP") != NULL)
            { //判断是不是http请求
                msg.buf = head;
                msg.fd = cfd;
                ret = pthread_create(&tid, NULL, deal_http, (void *)&msg);
                ret = pthread_detach(tid);
            }
            else
            {
                client_num++;
                fds[client_num].fd = cfd;
                fds[client_num].events = POLLIN;
                dup2(cfd, STDOUT_FILENO);
                printf("OK\r\n");
                dup2(STDOUT_FILENO, fdout);
            }
        }
        //  Acpt:
        tmp_num = client_num;
        for (i = 1; i < client_num+1; i++)//要把第一个除去毕竟我分开了的，其实可以加一块，只是沿用的select的设置
        {
            if (fds[i].revents == POLLIN)
            {
                memset(head, 0, sizeof(head));
                if (recv(fds[i].fd, head, sizeof(head), 0))
                { //先判断是不是已经断开了连接，如果已经断开了则销毁这个，如果没有则写入来的消息到LOG然后回写
                    dup2(log, STDOUT_FILENO);
                    fputs(head, stdout);
                    write(fds[i].fd, head, sizeof(head));
                    write(fds[i].fd, "OK\r\n", 4);
                    printf("OK\r\n");
                    dup2(STDOUT_FILENO, fdout);
                }
                else
                { //销毁之后还得把数组整体前移,且移出这个数组，并取消监听
                    shutdown(fds[i].fd, 2);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    tmp_num--;
                }
            }
        }
        for (i = 0; i < client_num+1; i++)
        {
            if (fds[i].fd == -1)
            {
                fds[i] = fds[i + 1];
                fds[i + 1].fd = -1;
            }
        }
        client_num = tmp_num;
        //if (ret != 0)
        //  ret--;
        // goto Acpt;
    }
    return 0;
}
