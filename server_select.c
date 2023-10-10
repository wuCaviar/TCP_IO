#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>

pthread_mutex_t mutex;

typedef struct fdinfo
{
    int fd;        // 文件描述符
    int *maxfd;     // 最大的文件描述符
    fd_set *rdset; // 读事件的集合
} FDInfo;

void *acceptConn(void *arg)
{
    printf("accept子线程ID：%ld\n", pthread_self());
    FDInfo *info = (FDInfo *)arg;
    // 接受连接请求
    int cfd = accept(info->fd, NULL, NULL); // 不关心客户端的地址信息
    if (cfd == -1)
    {
        perror("accept error");
        free(info);
        return NULL;
    }
    // 加锁
    pthread_mutex_lock(&mutex);
    FD_SET(cfd, info->rdset); // 将cfd添加到集合中
    // 更新最大的文件描述符

    *info->maxfd = *info->maxfd < cfd ? cfd : *info->maxfd; // maxfd = max(maxfd, cfd);
    // 解锁
    pthread_mutex_unlock(&mutex);

    free(info);

    return NULL;
}

void *communication(void *arg)
{
    printf("communication子线程ID：%ld\n", pthread_self());
    FDInfo *info = (FDInfo *)arg;

    // 读数据
    char buf[1024];
    int len = recv(info->fd, buf, sizeof(buf), 0);
    if (len == -1)
    {
        perror("read error");
        free(info);
        return NULL;
    }
    else if (len == 0)
    {
        printf("客户端已经断开了连接\n");
        // 加锁
        pthread_mutex_lock(&mutex);
        FD_CLR(info->fd, info->rdset); // 将i从集合中删除
        // 解锁
        pthread_mutex_unlock(&mutex);
        close(info->fd);
        free(info);
        return NULL;
    }
    printf("recv buf: %s\n", buf);
    // 小写转大写
    for (int i = 0; i < len; i++)
    {
        buf[i] = toupper(buf[i]);
    }
    printf("after buf: %s\n", buf);

    // 大写串发回客户端
    // write(cfd, buf, strlen(buf)+1);
    int ret = send(info->fd, buf, strlen(buf) + 1, 0);
    if (ret == -1)
    {
        perror("send error");
    }

    free(info);

    return NULL;
}

int main()
{
    // 创建互斥锁
    pthread_mutex_init(&mutex, NULL);

    // 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error");
        exit(1);
    }

    // 绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                // IPV4
    serv_addr.sin_port = htons(9999);              // 端口号
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // IP地址
    // 127.0.0.1
    // inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);

    // 绑定端口
    int ret = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1)
    {
        perror("bind error");
        exit(1);
    }

    // 设置监听
    ret = listen(lfd, 128); // 同一时刻允许向服务器发起128个连接请求
    if (ret == -1)
    {
        perror("listen error");
        exit(1);
    }

    fd_set redset;        // 读事件的集合
    FD_ZERO(&redset);     // 清空集合
    FD_SET(lfd, &redset); // 将lfd添加到集合中

    int maxfd = lfd; // 最大的文件描述符
    while (1)
    {
        // 加锁
        pthread_mutex_lock(&mutex);
        // select函数会改变集合，所以每次都要重新设置
        fd_set tmp = redset; // 临时集合等于redset
        // 解锁
        pthread_mutex_unlock(&mutex);
        // 第一个参数：最大的文件描述符+1
        // 第二个参数：读事件的集合
        // 第三个参数：写事件的集合
        // 第四个参数：异常事件的集合
        // 第五个参数：超时时间
        int ret = select(maxfd + 1, &tmp, NULL, NULL, NULL);
        // 判断是否是监听的fd
        if (FD_ISSET(lfd, &tmp))
        {
            // 创建子线程
            pthread_t tid;
            FDInfo *info = (FDInfo *)malloc(sizeof(FDInfo));
            info->fd = lfd;
            info->maxfd = &maxfd;
            info->rdset = &redset;
            pthread_create(&tid, NULL, acceptConn, info); // 接受连接
            pthread_detach(tid);
        }

        // 遍历集合中的文件描述符
        for (int i = 0; i < maxfd; ++i)
        {
            // 判断是否是有效的文件描述符
            if (i != lfd && FD_ISSET(i, &tmp))
            {        
                // 创建子线程
                pthread_t tid;
                FDInfo *info = (FDInfo *)malloc(sizeof(FDInfo));
                info->fd = i;
                info->rdset = &redset;
                pthread_create(&tid, NULL, communication, info); // 通信
                pthread_detach(tid);
            }
        }
    }

    close(lfd);

    pthread_mutex_destroy(&mutex);

    return 0;
}