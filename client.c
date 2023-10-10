#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

int main()
{
    // 1. 创建用于通信的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket");
        exit(0);
    }

    // 2. 连接服务器
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;     // ipv4
    addr.sin_port = htons(9999);   // 服务器监听的端口, 字节序应该是网络字节序
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    // 通信
    while(1)
    {
        //写数据
        char buf[1024];
        fgets(buf, sizeof(buf), stdin);
        ret = send(fd, buf, strlen(buf)+1, 0);
        if(ret == -1)
        {
            perror("send");
            exit(0);
        }
        printf("send buf: %s\n", buf);

        // 读数据
        int len = recv(fd, buf, sizeof(buf), 0);
        
        if(len == -1)
        {
            perror("recv");
            exit(0);
        }
        else if(len == 0)
        {
            printf("服务器断开了连接...\n");
            break;
        }
        printf("recv Buf: %s\n", buf);
    }

    // 释放资源
    close(fd); 

    return 0;
}
