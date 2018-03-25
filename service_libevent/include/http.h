#ifndef __HTTP_H_
#define __HTTP_H_

#define MAXLINE 1024
#include <stdio.h>
#include <event.h>
#include "../include/mylog.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef struct pthread_msg_http
{
    char *buf;
    bufferevent *bev;
} http_arg;

/*HTTP请求发送函数*/
void http_send_headers(char *type, long int length);
void http_send_err(int status, char *title, char *text);
void *deal_http(void *arg);

#ifdef __cplusplus
}
#endif
#endif