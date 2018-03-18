#ifndef __HTTP_H_
#define __HTTP_H_

#define MAXLINE 1024
#include <stdio.h>

typedef struct pthread_msg_http
{
    char *buf;
    int fd;
    FILE *tmp_in;
    FILE *tmp_out;
} pthread_msg_http;

/*HTTP请求发送函数*/
void http_send_headers(char *type, long int length, FILE *tmp_out);
void http_send_err(int status, char *title, char *text, FILE *tmp_out);




void *deal_http(void *arg);



#endif