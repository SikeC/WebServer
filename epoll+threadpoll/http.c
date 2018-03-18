#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <arpa/inet.h>
#include "sql.h"
#include "http.h"

void http_send_headers(char *type, off_t length, FILE *tmp_out)
{
    fprintf(tmp_out, "%s %d %s\r\n", "HTTP/1.1", 200, "0k");
    fprintf(tmp_out, "Content-Type:%s\r\n", type);
    fprintf(tmp_out, "Content-Length:%ld\r\n", (int64_t)length);
    fprintf(tmp_out, "Connection: close\r\n");
    fprintf(tmp_out, "\r\n");
}

void http_send_err(int status, char *title, char *text, FILE *tmp_out)
{
    http_send_headers("text/html", -1, tmp_out);

    fprintf(tmp_out, "<html><head><title>%d %s</title></head>\n", status, title);
    fprintf(tmp_out, "<body bgcolor=\"#cc99cc\"><h4>%d %s</h4>\n", status, title);
    fprintf(tmp_out, "%s\n", text);
    fprintf(tmp_out, "<hr>\n</body>\n</html>\n");
    fflush(tmp_out);
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
    char *parameter;
    FILE *fp;
    int fd_lock;
    int ret;
    int rc;
    int dynamic;
    char *fname;
    char *username;
    char *password;
    char command[MAXLINE];
    char *retpasswd[1];
    unsigned int timeout;
    MYSQL mysql;

    msg = (pthread_msg_http *)malloc(sizeof(pthread_msg_http));
    memset(msg, 0, sizeof(pthread_msg_http));
    memcpy(msg, arg, sizeof(pthread_msg_http));

    if (sscanf(msg->buf, "%[^ ] %[^ ] %[^ ]", method, path, protocol) != 3)
    {
        http_send_err(400, "Bad Request", "Can't parse request.", msg->tmp_out);
        goto end;
    }
    while (fgets(buf, sizeof(buf), msg->tmp_in) != NULL)
    {
        if (strcmp(buf, "\n") == 0 || strcmp(buf, "\r\n") == 0)
        {
            break;
        }
    }

    if (strcasecmp("GET", method) != 0 && strcasecmp("POST", method) != 0)
    {
        http_send_err(501, "Not Implemented", "That method is not implemented.", msg->tmp_out);
        goto end;
    }
    if (path[0] != '/')
    {
        http_send_err(400, "Bad Request", "Bad filename.", msg->tmp_out);
        goto end;
    }
    if (path[1] == '\0')
    {
        strcat(path, "html/index.html");
    }
    filesname = path + 1;
    if (strstr(filesname, ".info") != NULL)
    {
        //因为暂时不支持php等动态网页，就只能这样来
        fname = strsep(&filesname, "?");
        parameter = strsep(&filesname, "?");
        //分割字符串
        mysql_thread_init();
        if (mysql_init(&mysql) == NULL)
        {
            printf("mysql init failed: %s\n", mysql_error(&mysql));
            mysql_thread_end();
            free(msg);
            return (void *)0;
        }
        timeout = 3000;
        mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        if (!mysql_real_connect(&mysql, HOST, USERNAME, PASSWORD, DATABASE, 0, NULL, CLIENT_FOUND_ROWS)) //"root":数据库管理员 "":root密码 "test":数据库的名
        {
            mysql_close(&mysql);
            mysql_thread_end();
            return (void *)0;
        }

        if (strcmp(fname, "signin.info") == 0)
        {
            username = strsep(&parameter, "&");
            password = strsep(&parameter, "&");
            strsep(&username, "="); //剔除前面的username不要
            username = strsep(&username, "=");
            strsep(&password, "=");
            password = strsep(&password, "=");
            sprintf(command, "select password from user_registration_tbl where username='%s'", username);
            ret = select_sql(&mysql, command, retpasswd, rc);
            if (ret != 0)
            {
                http_send_err(200, username, "username invaild.", msg->tmp_out);
                mysql_thread_end();
                free(msg);
                goto end;
            }
            if (strcmp(retpasswd[0], password) == 0)
            {
                http_send_err(200, username, "login success.", msg->tmp_out);
            }
            else
            {
                http_send_err(200, username, "password invaild.", msg->tmp_out);
            }
            mysql_thread_end();
            goto end;
        }
        if (strcmp(fname, "signup.info") == 0)
        {
            username = strsep(&parameter, "&");
            password = strsep(&parameter, "&");
            strsep(&username, "="); //剔除前面的username不要
            username = strsep(&username, "=");
            strsep(&password, "=");
            password = strsep(&password, "=");
            sprintf(command, "INSERT INTO user_registration_tbl (username,password,creation_date) VALUES ('%s','%s',now())", username, password);
            ret = insert_sql(&mysql, command);
            if (ret != 0)
            {
                printf("mysql select failed: %s\n", mysql_error(&mysql));
                http_send_err(200, username, "logup error.", msg->tmp_out);
                mysql_thread_end();
                goto end;
            }
            http_send_err(200, username, "logup success.", msg->tmp_out);
            mysql_thread_end();
            goto end;
        }
    }
    if (stat(filesname, &s_buf) < 0)
    {
        http_send_err(404, "Not Found", "File not found.", msg->tmp_out);
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
        type = "image/jpeg;";
    }
    else if (strcmp(dot, ".gif") == 0)
    {
        type = "image/gif;";
    }
    else if (strcmp(dot, ".png") == 0)
    {
        type = "image/png;";
    }
    else if (strcmp(dot, ".mp3") == 0)
    {
        type = "audio/mpeg;";
    }
    else if (strcmp(dot, ".js") == 0)
    {
        type = "application/x-javascript;";
    }
    else if (strcmp(dot, ".css") == 0)
    {
        type = "text/css;";
    }
    else if (strcmp(dot, ".php") == 0)
    {
        dynamic = 1;
    }
    else
    {
        type = "text/plain;";
    }
    if (!dynamic)
    {
        parameter = strstr(filesname, "?");
        if (parameter != NULL)
        {
            *parameter = '\0';
        }
        fd_lock = open(filesname, O_RDONLY);
        ret = flock(fd_lock, LOCK_EX);
        if (ret == -1)
        {
            perror("flock error:");
            goto end;
        }
        fp = fdopen(fd_lock, "rb"); //以二进制打开文件，不然图片出错
        if (fp == NULL)
        {
            http_send_err(403, "Forbidden", "File is protected.", msg->tmp_out);
            fclose(fp);
            goto end;
        }
        http_send_headers(type, s_buf.st_size, msg->tmp_out);

        memset(buf, -1, sizeof(buf));

        while ((rc = fread(buf, sizeof(char), MAXLINE * 4, fp)) != 0) //写入的时候一样的，用二进制写入
        {
            fwrite(buf, sizeof(char), MAXLINE * 4, msg->tmp_out);
        }

        fflush(msg->tmp_out);

        fclose(fp);
        ret = flock(fd_lock, LOCK_UN);
        if (ret == -1)
        {
            perror("flock error");
            goto end;
        }
    }
    else
    {
        filesname = strtok(filesname, "?");
        *parameter = '\0'; //分割字符串,把？用结束符替换
        parameter += 1;
        if (stat(filesname, &s_buf) < 0)
        {
            http_send_err(404, "Not Found", "File not found.", msg->tmp_out);
            goto end;
        }
        //这个可以以后慢慢加上
    }
end:
    fclose(msg->tmp_in);
    fclose(msg->tmp_out);
    close(msg->fd);
    free(msg);

    return NULL;
}