#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

int main()
{
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

    // 设置端口复用
    int optval = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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

    // 创建epoll树的根节点
    int epfd = epoll_create(1); // 参数无意义，只要大于0就行
    if (epfd == -1)
    {
        perror("epoll_create error");
        exit(1);
    }

    // 初始化epoll树
    struct epoll_event ev;

    ev.events = EPOLLIN | EPOLLET; // 边沿触发

    // ev.events = EPOLLIN; // 无边沿触发
    ev.data.fd = lfd; // 将lfd挂到epoll树上
    // 第一个参数是epoll树的根节点
    // 第二个参数是对lfd的操作
    // 第三个参数是对lfd的描述
    // 第四个参数是对lfd的操作，可选的操作有：EPOLL_CTL_ADD：注册新的fd到epfd中；EPOLL_CTL_MOD：修改已经注册的fd的监听事件；EPOLL_CTL_DEL：从epfd中删除一个fd；
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev); // 将lfd挂到epoll树上

    // 定义一个数组，用来接收epoll_wait返回的结果
    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(evs[0]);
    while (1)
    {
        // 第一个参数是epoll树的根节点
        // 第二个参数是接收epoll_wait返回的结果
        // 第三个参数是接收epoll_wait返回的结果的个数
        // 第四个参数是超时时间，-1表示阻塞等待
        int num = epoll_wait(epfd, evs, size, -1); // 阻塞监听
        printf("num = %d\n", num);
        for (int i = 0; i < num; i++)
        {
            int fd = evs[i].data.fd;
            if (fd == lfd)
            {
                int cfd = accept(fd, NULL, NULL);

                int flag = fcntl(cfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flag);

                ev.events = EPOLLIN | EPOLLET; // 边沿触发
                // ev.events = EPOLLIN; // 关注读事件
                ev.data.fd = cfd; // 将lfd挂到epoll树上
                // 第一个参数是epoll树的根节点
                // 第二个参数是对lfd的操作
                // 第三个参数是对lfd的描述
                // 第四个参数是对lfd的操作，可选的操作有：EPOLL_CTL_ADD：注册新的fd到epfd中；EPOLL_CTL_MOD：修改已经注册的fd的监听事件；EPOLL_CTL_DEL：从epfd中删除一个fd；
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev); // 将lfd挂到epoll树上
            }
            else
            {
                char buf[5]; // 一次性读5个字节

                while (1)
                {
                    int len = recv(fd, buf, sizeof(buf), 0);
                    if (len == -1)
                    {
                        if (errno == EAGAIN)
                        {
                            printf("数据已经接收完毕.....\n");
                            break;
                        }
                        else
                        {
                            perror("read error");
                            exit(1);
                        }
                    }
                    else if (len == 0)
                    {
                        printf("客户端已经断开了连接...\n");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        break;
                    }
                    printf("recv buf: %s\n", buf);

                    // 小写转大写
                    for (int i = 0; i < len; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                    // write(1, buf, len);
                    printf("after buf: %s\n", buf);

                    // write data
                    // write(cfd, buf, strlen(buf) + 1);
                    ret = send(fd, buf, strlen(buf) + 1, 0);
                    if (ret == -1)
                    {
                        perror("send error");
                        exit(1);
                    }
                }

                // read data
                // int len = read(cfd, buf, sizeof(buf));
                // int len = recv(fd, buf, sizeof(buf), 0);
            }
        }
    }

    close(lfd);

    return 0;
}