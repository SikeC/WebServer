#include <stdio.h>
#include <string.h>

#include "sql.h"
#include "mysql/mysql.h"

int insert_sql(MYSQL *conn, char *command)
{
    int res;
    res = mysql_real_query(conn, command, strlen(command));
    if (res)
    {
        printf("mysql_insert error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return -1;
    }
    else
    {
        printf("mysql_insert OK\n");
    }
    mysql_close(conn);
    return 0;
}

int select_sql(MYSQL *conn, char *command, char **buf, int num)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW rows;
    int fields;
    int i;

    mysql_real_query(conn, command, strlen(command));
    res = mysql_store_result(conn);
    if (NULL == res)
    {
        printf("mysql_restore_result(): %s\n", mysql_error(conn));
        mysql_close(conn);
        mysql_library_end();
        return -1;
    }
    fields = mysql_num_fields(res);
    while ((rows = mysql_fetch_row(res)))
    {
        for (i = 0; i < fields; i++)
        {
            buf[i] = (char*)malloc(sizeof(char)*strlen(rows[i]));
            memcpy(buf[i],rows[i],strlen(rows[i]));
        }
    }
    num = fields;
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}
