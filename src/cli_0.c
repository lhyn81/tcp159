#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

char* SERVER_IP;
int SERVER_PORT;
int NUM;

char *DATA;
uint8_t INDEX=0;


void save()
{
    const char* fn = "data.bin";
    FILE* file = fopen(fn, "w");
    if (file == NULL) {
        printf("无法打开文件 %s\n", fn);
        exit(1);
    }

    fwrite(DATA, 1, (NUM*3*sizeof(float)+2), file);
    fclose(file);
}

int main(int argc, char *argv[]) {

    SERVER_IP = argv[1];
    SERVER_PORT = atoi(argv[2]);
    NUM = atoi(argv[3]);

    if (argc != 4) {
        printf("参数不正确,用法如下:\n");
        printf("%s 服务端IP 服务端端口\n", argv[0]);
        return 1;
    }


    int sockfd;
    struct sockaddr_in server_addr;

    // 创建TCP套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    // 连接服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    DATA=(char*)malloc(NUM*3*sizeof(float)+2);
    DATA[0]=0xFF;
    DATA[NUM*3*sizeof(float)+1]=0xFF;
    // printf("Data size if %ld\n",(NUM*3*sizeof(float)+2));

    while (1) {
        // 发送数据到服务器
        const char* send_msg = "A\r";
        ssize_t send_len = send(sockfd, send_msg, strlen(send_msg), 0);
        if (send_len < 0) {
            perror("send");
            exit(1);
        }


        // 接收服务器的响应数据
        char recv_buf[12 + 1];  // 12个字节的缓冲区
        ssize_t recv_len = recv(sockfd, recv_buf, 12, 0);
        if (recv_len < 0) {
            perror("recv");
            exit(1);
        } else if (recv_len == 0) {
            printf("Server disconnected\n");
            exit(1);
        }

        // 将接收到的字节解析为3个浮点数
        float nums[3];
        if (INDEX>=NUM)
        {
            printf("1 cycle done.\n");
            save();
            INDEX=0;
        } 
        memcpy(DATA+1+INDEX*3*sizeof(float), recv_buf, sizeof(float) * 3);
        INDEX++;
        

        // 打印解析后的浮点数
        // printf("Received numbers: %f, %f, %f\n", nums[0], nums[1], nums[2]);

    }

    // 关闭套接字
    close(sockfd);

    return 0;
}
