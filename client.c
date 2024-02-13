#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define N 32

// MSG种类
#define R 1 // 注册
#define L 2 // 登录
#define Q 3 // 查询
#define H 4 // 历史

// 定义通信双方的信息结构体
typedef struct msg
{
    /* data */
    int type;
    char name[N];
    char data[512];
} MSG;

// 注册
// 客户端向服务起发送注册数据 服务器给客户端反馈
int do_register(int sockfd, MSG *msg)
{
    msg->type = R;
    printf("Input name:");
    scanf("%s", msg->name);
    getchar();
    printf("Input password:");
    scanf("%s", msg->data);
    getchar();
    if (-1 == send(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    if (-1 == recv(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("接收失败!!!\n");
        return -1;
    }
    // 打印一下接收的数据 ok! 或者是 already exist
    printf("%s\n", msg->data);
    return 0;
}

// 登录 发送登录信息 接收返回值并判断
int do_login(int sockfd, MSG *msg)
{
    msg->type = L;
    printf("Input name:");
    scanf("%s", msg->name);
    getchar();
    printf("Input password:");
    scanf("%s", msg->data);
    getchar();
    if (-1 == send(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    if (-1 == recv(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("接收失败!!!\n");
        return -1;
    }
    // 比较登录返回的数据OK或者是fail
    if (strncmp(msg->data, "OK", 3) == 0)
    {
        /* code */
        return 1;
    }
    return 0;
}

// 查询单词
int do_query(int sockfd, MSG *msg)
{
    msg->type = Q;
    printf("--------------------\n");
    while (1)
    {
        /* code */
        printf("Input word:");
        scanf("%s", msg->data);
        getchar();
        if (0 == strncmp(msg->data, "#", 2))
        {
            /* code */
            break;
        }
        // 将要查询的单词发送给服务器
        if (-1 == send(sockfd, msg, sizeof(MSG), 0))
        {
            /* code */
            printf("发送失败!!!\n");
            continue;
        }
        // 等待服务器返回数据
        if (-1 == recv(sockfd, msg, sizeof(MSG), 0))
        {
            /* code */
            printf("接收失败!!!\n");
            continue;
        }
        printf("%s\n", msg->data);
    }
    return 0;
}

// 获得历史记录
int do_history(int sockfd, MSG *msg)
{
    msg->type = H;
    // 发送信息MSG
    if (-1 == send(sockfd, msg, sizeof(MSG), 0))
    {
        /* code */
        printf("发送失败!!!\n");
        return -1;
    }
    // 接收服务端的消息
    while (1)
    {
        /* code */
        // 读取客户端发送过来的用户查询记录
        if (-1 == recv(sockfd, msg, sizeof(MSG), 0))
        {
            /* code */
            printf("接收失败!!!\n");
            return -1;
        }
        else
        {
            /* code */
            if ('\0' == msg->data[0])
            {
                /* code */
                break;
            }
            printf("%s\n", msg->data);
        }
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    /* code */
    int sockfd;
    int choice;
    struct sockaddr_in serveraddr;
    MSG msg;
    if (3 != argc)
    {
        /* code */
        printf("Usage:%s serverip port.\n", argv[0]);
        exit(-1);
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
    // 连接服务器
    if (-1 == connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
        /* code */
        perror("connect error!");
        exit(-1);
    }
    // 显示菜单
    while (1)
    {
        /* code */
        // 一级菜单
        printf("********************************************************************************\n");
        printf("1.注册                               2.登录                                 3.退出\n");
        printf("********************************************************************************\n");
        printf("请选择:");
        scanf("%d", &choice);
        // 去掉垃圾字符回车
        getchar();
        switch (choice)
        {
        case 1:
            /* code */
            // 注册相关代码
            do_register(sockfd, &msg);
            break;
        case 2:
            /* code */
            // 二级菜单
            if (1 == do_login(sockfd, &msg))
            {
                // 进入二级菜单
                printf("登录成功!!!\n");
                goto next;
            }
            else
            {
                /* code */
                printf("登录失败!!!\n");
                printf("请确认是否注册!!!账号或密码是否正确!!!\n");
            }
            break;
        case 3:
            /* code */
            // 退出进程
            close(sockfd);
            exit(0);
            break;
        default:
            printf("输入有误!!请输入以上三个选项!!\n");
            break;
        }
    }
next:
    while (1)
    {
        /* code */
        // 二级菜单
        printf("********************************************************************************\n");
        printf("1.查询单词                              2.历史记录                           3.退出\n");
        printf("********************************************************************************\n");
        printf("请选择:");
        scanf("%d", &choice);
        // 去掉垃圾字符回车
        getchar();
        switch (choice)
        {
        case 1:
            /* code */
            // 查询单词
            if (-1 == do_query(sockfd, &msg))
            {
                /* code */
                printf("查询失败!!!\n");
            }
            break;
        case 2:
            /* code */
            // 得到当前用户的历史记录
            if (-1 == do_history(sockfd, &msg))
            {
                /* code */
                printf("没有历史记录!!!\n");
            }
            break;
        case 3:
            /* code */
            // 退出进程
            close(sockfd);
            exit(0);
            break;
        default:
            printf("输入有误!!请输入以上三个选项!!\n");
            break;
        }
    }

    return 0;
}
