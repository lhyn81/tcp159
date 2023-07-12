#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

// Save mode
int mode;

// Server config
char* SERVER_IP;
int SERVER_PORT;

// Realtime Data buffer config
char *DATA;
uint8_t INDEX=0;
int NUM;

// History Data buffer config
char* dynData;

// Current time var
time_t curTime;
struct tm * timeInfo;
char dayStr[10]="0";
char secStr[10]="0";

// Parse float
void parseByteArray(const unsigned char* byteArray, float* floatArray) {
    int i;
    for (i = 0; i < 3; i++) {
        unsigned char* tempArray = malloc(4);
        memcpy(tempArray, byteArray + (i * 4), 4);
        floatArray[i] = *((float*)tempArray);
        free(tempArray);
    }
}

void formatFloatArray(const float* floatArray, char* resultString) {
    sprintf(resultString, "%.2e,%.2e,%.2e\n", floatArray[0], floatArray[1], floatArray[2]);
}

// Save realtime data
void save_rt()
{
    const char* fn = "data.bin";
    FILE* file = fopen(fn, "w");
    if (file == NULL) {
        printf("无法打开文件 %s\n", fn);
        exit(1);
    }
    fwrite(DATA, 1, (NUM*(3*sizeof(float)+2)+2), file);
    fclose(file);
}

// Save history data
void save_his()
{
    char dataDir[20] = "data";
    int result = mkdir(dataDir, 0777);
    strcat(dataDir,"/");
    strcat(dataDir,dayStr);
    result = mkdir(dataDir, 0777);
    strcat(dataDir,"/");
    strcat(dataDir,secStr);
    strcat(dataDir,".txt");
    FILE* file = fopen(dataDir, "w");
    if (file != NULL) {
        fprintf(file, "%s", dynData);
        fclose(file);
        printf("数据写入%s.txt\n",secStr);
        dynData = realloc(dynData, 1);
        *dynData = '\0';
    } else {
        printf("无法打开文件\n");
    }
}

// Main
int main(int argc, char *argv[]) {

    // Param check
    if (argc != 4) {
        printf("参数不正确,用法如下:\n");
        printf("%s 服务端IP 服务端Port 0或者50~250之间的整数\n", argv[0]);
        return 1;
    }

    // Read params
    SERVER_IP = argv[1];
    SERVER_PORT = atoi(argv[2]);
    NUM = atoi(argv[3]);
    if (NUM==0)
    {
        mode = 1; // History mode
        dynData = malloc(1);
        *dynData = '\0';
    }
    else
    {
        mode = 0; // Readtime mode
        if (NUM<50 | NUM>250)
        {
            printf("单次读取条目数介于50-250之间\n");
            return 1;
        }
    }

    // Socket Init
    int sockfd;
    struct sockaddr_in server_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket创建错误\n");
        exit(1);
    }

    // Config server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    // Connect
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("服务器连接错误\n");
        exit(1);
    }

    // Allocate data buffer
    DATA=(char*)malloc(NUM*(3*sizeof(float)+2)+2);
    DATA[0]=0xFF;
    DATA[NUM*(3*sizeof(float)+2)+1]=0xFF;

    switch (mode)
    {
    case 0:
        printf("实时模式: 数据写入data.bin\n");
        printf("按下Ctrl+C中断程序.\n");
        break;
    
    case 1:
        printf("历史模式: 数据写入data文件\n");
        printf("按下Ctrl+C中断程序.\n");
        break;
    }
    while (1) 
    {
        // Send cmd
        unsigned char cmd[2] = {0x51, 0x0A};
        // const char* send_msg = "A\n";
        ssize_t send_len = send(sockfd, cmd, sizeof(cmd), 0);
        if (send_len < 0) {
            perror("发送命令出错.\n");
            exit(1);
        }


        // Receive 12 bytes data.
        char recv_buf[14];  // 12个字节的缓冲区
        ssize_t recv_len = recv(sockfd, recv_buf, 14, 0);
        // printf("%ld\n",recv_len);
        if (recv_len < 0) {
            perror("接收数据出错.\n");
            exit(1);
        } else if (recv_len == 0) {
            printf("服务器中断.\n");
            exit(1);
        }

        // Switch mode
        switch(mode)
        {
            case(0):
            {
                if (INDEX>=NUM)
                {
                    save_rt();
                    INDEX=0;
                } 
                memcpy(DATA+1+INDEX*3*sizeof(float), recv_buf, sizeof(float) * 3);
                INDEX++;
                break;
            }
            case(1):
            {
                // expand dynData
                float floatArray[3];
                parseByteArray(recv_buf, floatArray);
                char resultString[50];
                formatFloatArray(floatArray, resultString);
                size_t len = strlen(dynData);
                size_t timeLen = strlen(resultString);
                dynData = realloc(dynData, len + timeLen + 1 + 1);
                strcat(dynData,resultString);

                // check curTime 
                curTime = time(NULL);
                timeInfo = localtime(&curTime);
                strftime(dayStr, sizeof(dayStr), "%Y%m%d", timeInfo);
                char newSec[10];
                strftime(newSec, sizeof(newSec), "%H-%M", timeInfo);
                if (strcmp(secStr,newSec)!=0)
                {
                    save_his();
                    strcpy(secStr,newSec);
                }
                break;
            }
        }
        // Reach the max count reset to 0
        // 打印解析后的浮点数
        // float nums[3];
        // printf("Received numbers: %f, %f, %f\n", nums[0], nums[1], nums[2]);
    }

    close(sockfd);

    return 0;
}
