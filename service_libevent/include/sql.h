#ifndef __SQL_H_
#define __SQL_H_

#include "mysql/mysql.h"

#ifdef __cplusplus
extern "C"{
#endif

#define HOST "192.168.1.103"
#define USERNAME "root"
#define PASSWORD "cy%960811"
#define DATABASE "Account_Information"

int insert_sql(MYSQL *conn, char *command);
int select_sql(MYSQL *conn, char *command, char **buf, int num);

#ifdef __cplusplus
}
#endif
#endif