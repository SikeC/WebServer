#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <pthread.h>
#include <string.h>

#include "wrap.h"

#define MAXLINE 1024
#define SERV_PORT 8888
#define SERV_IP "192.168.1.102"

char head[MAXLINE];

int fdin;
int fdout;

typedef struct pthread_msg_http
{
    char *buf;
    int fd;
} pthread_msg_http;

//自己写的一个动态数组，感觉就是有压力一样在数组尾部把空的位置直接挤压出去
/*
int CreatPressureArray(void ***array, size_t size, int n)
{
    void **tmp;
    if (array == NULL || size == 0)
    {
        printf("Func CreatPressureArray error: Invalid parameter\r\n");
        return -1;
    }
    tmp = calloc(n, size);
    tmp[0] = NULL;
    *array = tmp;
    return 0;
}*/

int SetPressureArray(void *array[], void *node)
{
    int i;
    void **tmp;

    if (array == NULL || node == NULL)
    {
        printf("Func SetPressureArray error: Invalid parameter\r\n");
        return -1;
    }

    tmp = array;

    for (i = 0; tmp[i] != NULL; i++)
        ;
    tmp[i] = node;
    tmp[i + 1] = NULL;

    return 0;
}

int DelPressureArray(void *array[], void *node)
{
    int i;
    void **tmp;

    if (array == NULL || node == NULL)
    {
        printf("Func DelPressureArray error: Invalid parameter\r\n");
        return -1;
    }

    tmp = array;

    for (i = 0; tmp[i] != NULL; i++)
    {
        ;
        if (tmp[i] == node)
        {
            break;
        }
    }
    if (tmp[i] == NULL)
    {
        printf("Func DelPressureArray error: Invalid node\r\n");
        return -1;
    }
    tmp[i] = NULL;
    for (i++; tmp[i] != NULL; i++)
    {
        tmp[i - 1] = tmp[i];
    }
    tmp[i--] = NULL;

    return 0;
}

int DestroyPressureArray(void *array[])
{
    void **tmp;
    int i;

    if (array == NULL)
    {
        printf("Func DestroyPressureArray error: Invalid parameter\r\n");
        return -1;
    }

    tmp = array;

    free(tmp);
    tmp = NULL;

    return 0;
}

int SortPressureArray(void *array[], int *max, int *min)
{ //用插入法排序，可求出最大最小值，用于整形为成员时，如max,min传NULL则此值不传出，可仅排序，例如：SortPressureArray(&array)||SortPressureArray(&array,&max)||SortPressureArray(&array,&max,&min)
    int i, j;
    int *key;
    void **tmp;
    int *buf;

    if (array == NULL)
    {
        printf("Func SortPressureArray error: Invalid parameter\r\n");
        return -1;
    }

    tmp = array;
    for (j = 1; tmp[j] != NULL; j++)
    {
        key = (int *)tmp[j];
        i = j - 1;
        buf = (int *)tmp[i];
        while (i >= 0 && *buf > *key)
        {
            tmp[i + 1] = tmp[i];
            i = i - 1;
            buf = (int *)tmp[i];
        }
        tmp[i + 1] = key;
    }
    if (min != NULL)
    {
        buf = tmp[0];
        *min = *buf;
    }
    if (max != NULL)
    {
        buf = tmp[j - 1];
        *max = *buf;
    }

    return 0;
}

int IsPressureArrayEmpty(void *array[])
{
    void **tmp;
    if (array == NULL)
    {
        printf("Func IsPressureArrayEmpty error: Invalid parameter\r\n");
        return -1;
    }
    tmp = array;
    return tmp[0] == NULL;
}

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
    int lfd, cfd, maxfd, i, client_num, tmp_num;
    int *tmpfd;
    unsigned long serv_ip;
    int ret;
    fd_set readset, tmpset;
    int log;
    int *list[MAXLINE];
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

    FD_ZERO(&readset);
    FD_ZERO(&tmpset);
    FD_SET(lfd, &readset);
    tmpset = readset;
    maxfd = lfd;

    client_num = 0;

    fputs("Web Service Running ...\r\n", stdout);

   /* ret = CreatPressureArray((void ***)&list, sizeof(int), MAXLINE); //初始化监听fd数组
    if (ret == -1)
    {
        return ret;
    }*/

    list[0]=NULL;

    FD_SET(cfd, &readset);

    while (1)
    {
        if (!IsPressureArrayEmpty((void*)&list))
        {
            ret = SortPressureArray((void *)&list, (void *)&maxfd, NULL); //重新进行排序，保证maxfd
            if (ret == -1)
            {
                return ret;
            }
        }
        else
        {
            maxfd = lfd;
        }
        readset = tmpset;
        ret = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (ret == -1)
        {
            perror("select error:");
        }
        if (FD_ISSET(lfd, &readset))
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
                tmpfd = malloc(sizeof(int));
                *tmpfd = cfd;
                ret = SetPressureArray((void *)&list, (void *)tmpfd); //添加到新的连接里面然后检测
                if (ret == -1)
                {
                    return ret;
                }
                client_num++;
                FD_SET(cfd, &tmpset);
                dup2(cfd, STDOUT_FILENO);
                printf("%d\r\n",client_num);
                printf("%ls\r\n",list[client_num-1]);
                printf("OK\r\n");
                dup2(STDOUT_FILENO, fdout);
            }
        }
        //  Acpt:
	tmp_num = client_num;
        for (i = 0; i < client_num; i++)
        {
            if (FD_ISSET(*list[i], &readset))
            {
                memset(head, 0, sizeof(head));
                if (recv(*list[i], head, sizeof(head), 0))
                { //先判断是不是已经断开了连接，如果已经断开了则销毁这个，如果没有则写入来的消息到LOG然后回写
                    dup2(log, STDOUT_FILENO);
                    fputs(head, stdout);
                    write(*list[i], head, sizeof(head));
                    write(*list[i], "OK\r\n", 4);
                    printf("OK\r\n");
                    dup2(STDOUT_FILENO, fdout);
                }
                else
                { //销毁之后还得把数组整体前移,且移出这个数组，并取消监听
                    FD_CLR(*list[i], &tmpset);
                    shutdown(*list[i], 2);
                    close(*list[i]);
                    tmpfd = list[i];
                    DelPressureArray((void *)&list, (void *)list[i]);
                    free(tmpfd);
                    tmpfd = NULL;
                    tmp_num--;
                }
            }
        }
	client_num = tmp_num;
        //if (ret != 0)
        //  ret--;
        // goto Acpt;
    }
    return 0;
}
 
