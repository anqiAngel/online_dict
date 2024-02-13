#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define N 32

// MSG种类
#define R 1 // 注册
#define L 2 // 登录
#define Q 3 // 查询
#define H 4 // 历史

#define DATABASE "my.db"

// 定义通信双方的信息结构体
typedef struct msg
{
    /* data */
    int type;
    char name[N];
    char data[512];
} MSG;

// 函数声明
int do_client(int sockfd, sqlite3 *db);
int do_register(int sockfd, MSG *msg, sqlite3 *db);
int do_login(int sockfd, MSG *msg, sqlite3 *db);
int do_query(int sockfd, MSG *msg, sqlite3 *db);
int do_history(int sockfd, MSG *msg, sqlite3 *db);
int do_searchword(MSG *msg, char *word);
int get_date(char *date);
int history_callback(void *arg, int columns, char **values, char **name);

int main(int argc, char const *argv[])
{
    /* code */
    int sockfd;
    int choice;
    struct sockaddr_in serveraddr;
    MSG msg;
    sqlite3 *db;
    int acceptfd;
    if (3 != argc)
    {
        /* code */
        printf("Usage:%s serverip port.\n", argv[0]);
        exit(-1);
    }
    // 打开sqlite3数据库
    if (SQLITE_OK != sqlite3_open(DATABASE, &db))
    {
        /* code */
        printf("%s\n", sqlite3_errmsg(db));
        exit(-1);
    }
    else
    {
        /* code */
        printf("open database sucess!!\n");
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        /* code */
        perror("socket error");
        exit(-1);
    }
    // 清空地址结构体
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    // 将点分十进制转换为网络的二进制数据
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    // 将字符串转换为整形再转换为网络字节序
    serveraddr.sin_port = htons(atoi(argv[2]));
    // 给套接字绑定端口
    if (-1 == bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
        /* code */
        perror("bind error!");
        exit(-1);
    }
    // 将套接字设为监听模式 listen等待客户端连接
    if (-1 == listen(sockfd, 5))
    {
        /* code */
        perror("listen error!");
        exit(-1);
    }
    // 处理僵尸进程
    signal(SIGCHLD, SIG_IGN);
    // 服务器进入监听状态等待客户端连接
    while (1)
    {
        /* code */
        // 不想知道客户端的地址和地址大小后面两个参设为NULL
        acceptfd = accept(sockfd, NULL, NULL);
        if (-1 == acceptfd)
        {
            /* code */
            perror("accept error!");
            exit(-1);
        }
        else
        {
            /* code */
            // 创建子进程
            switch (fork())
            {
            case -1:
                /* code */
                perror("fork error!");
                exit(-1);
                break;
            case 0:
                /* code */
                // 子进程
                // 子进程只需要与客户端通信的套接字 将监听套接字清理掉
                close(sockfd);
                // 处理客户端请求
                do_client(acceptfd, db);
                break;
            default:
                // 父进程
                // 子进程只需要与客户端通信的套接字 将与客户端通信的套接字清理掉
                close(acceptfd);
                break;
            }
        }
    }

    return 0;
}

// 处理客户端请求
int do_client(int sockfd, sqlite3 *db)
{
    MSG msg;
    // 接收客户端的数据
    while (-1 != recv(sockfd, &msg, sizeof(msg), 0))
    {
        /* code */
        switch (msg.type)
        {
        case R:
            /* code */
            do_register(sockfd, &msg, db);
            break;
        case L:
            /* code */
            do_login(sockfd, &msg, db);
            break;
        case Q:
            /* code */
            do_query(sockfd, &msg, db);
            break;
        case H:
            /* code */
            do_history(sockfd, &msg, db);
            break;
        default:
            printf("无效的消息!!!\n");
            break;
        }
    }
    // 客户端已经断开连接
    // 打印客户端退出
    printf("客户端断开连接!!!\n");
    // 关闭套接字
    close(sockfd);
    // 退出子线程
    exit(0);
    return 0;
}

// 注册
// 得到数据查询数据库
int do_register(int sockfd, MSG *msg, sqlite3 *db)
{
    char *errmsg;
    char sql[128];
    sprintf(sql, "insert into usr values ('%s','%s')", msg->name, msg->data);
    printf("%s\n", sql);
    if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        /* code */
        printf("%s\n", errmsg);
        strcpy(msg->data, "用户已经存在!!!");
    }
    else
    {
        /* code */
        printf("客户注册成功!!!\n");
        strcpy(msg->data, "注册成功!!!\n");
    }
    if (-1 == send(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    return 0;
}

// 登录
int do_login(int sockfd, MSG *msg, sqlite3 *db)
{
    char *errmsg;
    char sql[128];
    int nrow;
    int ncolumn;
    char **resultp;
    sprintf(sql, "select * from usr where name = '%s' and pass = '%s'", msg->name, msg->data);
    printf("%s\n", sql);
    if (SQLITE_OK != sqlite3_get_table(db, sql, &resultp, &nrow, &ncolumn, &errmsg))
    {
        /* code */
        printf("%s\n", errmsg);
        return -1;
    }
    else
    {
        /* code */
        printf("SQL执行成功!!!\n");
        // 查询结果有一行则拥有此用户
        if (1 == nrow)
        {
            /* code */
            // 登录成功
            strcpy(msg->data, "OK");
        }
        else
        {
            /* code */
            // 没有改用户或用户名密码错误
            // 登录失败
            strcpy(msg->data, "FAIL");
        }
    }
    if (-1 == send(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    return 1;
}

// 查询单词
int do_query(int sockfd, MSG *msg, sqlite3 *db)
{
    char *errmsg;
    char word[64];
    int found = 0;
    // 定义一个时间
    char date[128];
    // 定义SQL
    char sql[128];
    // 拿出要查询的单词
    strcpy(word, msg->data);
    found = do_searchword(msg, word);
    if (1 == found)
    {
        /* code */
        // 获取系统时间
        get_date(date);
        sprintf(sql, "insert into record values ('%s','%s','%s')", msg->name, date, word);
        printf("%s\n", sql);
        // 将这条记录添加到record记录表中
        if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errmsg))
        {
            /* code */
            printf("%s\n", errmsg);
        }
    }
    else
    {
        /* code */
        strcpy(msg->data, "没找到此单词!!!");
    }
    // 信息发送给客户端
    if (-1 == send(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    return 0;
}

// 获得历史记录
int do_history(int sockfd, MSG *msg, sqlite3 *db)
{
    char *errmsg;
    char sql[128];
    sprintf(sql, "select * from record where name = '%s'", msg->name);
    printf("%s\n", sql);
    // 查询使用回调函数
    if (SQLITE_OK != sqlite3_exec(db, sql, history_callback, (void *)&sockfd, &errmsg))
    {
        /* code */
        printf("%s\n", errmsg);
        return -1;
    }
    else
    {
        /* code */
        printf("SQL执行成功!!!\n");
    }
    msg->data[0] = '\0';
    if (-1 == send(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    return 0;
}

// 查询单词
int do_searchword(MSG *msg, char *word)
{
    // 打开文件
    FILE *fp;
    char temp[512] = {0};
    int result = 0;
    fp = fopen("./dict.txt", "r");
    char *p;
    if (NULL == fp)
    {
        /* code */
        printf("文件打开失败!!!\n");
        strcpy(msg->data, "文件打开失败!!!");
        return -1;
    }
    // 打印出客户端要查询的单词
    int len = strlen(word);
    printf("%s , len = %d\n", word, len);
    // 读文件查询单词
    while (NULL != fgets(temp, 512, fp))
    {
        /* code */
        // 比较每一次读取的结果(一行)
        result = strncmp(word, temp, len);
        // 结果处理
        // temp > word dict.txt有一个规律后面的单词比前面的大就strncmp而言 后面就没有了直接跳出循环没找到没有
        if (result < 0)
        {
            /* code */
            break;
        } // word > temp 没找到继续向下
        else if (result > 0)
        {
            /* code */
            continue;
        }
        else
        {
            /* code */
            if (' ' != temp[len])
            {
                /* code */
                break;
            }
            else
            {
                /* code */
                // 找到了word 把注释挑出来
                // 让p指向第一个空格
                p = temp + len;
                // 去除空格
                while (' ' == *p)
                {
                    /* code */
                    p++;
                }
                // p就是注释的第一个字符
                // 把注释传给msg->data
                strcpy(msg->data, p);
                // 拷贝完毕关闭文件
                fclose(fp);
                return 1;
            }
        }
    }

    fclose(fp);
    return 0;
}

// 获取时间
int get_date(char *date)
{
    time_t t;
    time(&t);
    // 进行需要格式的转换
    struct tm *tp = localtime(&t);
    sprintf(date, "%d-%d-%d %d:%d:%d", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    return 0;
}

// 回调函数
// 得到查询结果并将结果发送给客户端
int history_callback(void *arg, int columns, char **values, char **name)
{
    int acceptfd;
    acceptfd = *((int *)arg);
    MSG msg;
    sprintf(msg.data, "%s , %s", values[1], values[2]);
    if (-1 == send(acceptfd, &msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    return 0;
}