#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define SOCKET int
#define MAX_BUF 1024
#define SERVERADDR "127.0.0.1"
#define PORT "5001"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serialize.h"

// Current time modifier
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"

// Addition funtion
void print_time();
int random_ue_id(int minN, int maxN);

// Return a connected socket to the gnb
SOCKET get_connect_socket();

// Send and receive msg function
void messaging(SOCKET socketfd, int ue_id, int loopcount, int msg_per_loop);

// message
/*
 * ------------------------------
 * Main function
 * ------------------------------
 */
int main(int argc, char *argv[])
{

    SOCKET socketfd = get_connect_socket();
    printf("Socket fd number: %d\n", socketfd);
    // Start communication
    printf("Start simulating...\n");

    int RRC_Att = 0, RRC_Succ = 0;
    srand((int)time(0));

    int ue_id = random_ue_id(100, 999);
    int loop = 5, msg_per_loop = 10;
    messaging(socketfd, ue_id, loop, msg_per_loop);

    return 0;
}

SOCKET get_connect_socket()
{
    // Get address info
    printf("Get address info....\n");
    struct addrinfo hints, *res;
    int status;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(SERVERADDR, PORT, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(status));
        exit(1);
    }

    // Create Socket
    printf("Creating socket....\n");
    SOCKET socketfd;
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // Check valid
    if (!ISVALIDSOCKET(socketfd))
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        exit(1);
    }

    // Connect
    if (connect(socketfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        close(socketfd);
        perror("client: connect");
        exit(1);
    }
    // Free addrinfo after binding socket
    freeaddrinfo(res);

    return socketfd;
}

void messaging(SOCKET socketfd, int ue_id, int loopcount, int msg_per_loop)
{
    for (int j = 0; j < loopcount; j++)
    {
        double interval = 1;       // 10 msg per second
        time_t start = time(NULL); // Start
        unsigned char buffer[MAX_BUF];
        RRCSetupRequest msg1 = {1, ue_id, 3};

        int i = 0;
        while (i < msg_per_loop)
        {
            // MSG1 RRCSetupRequest
            memset(buffer, 0, MAX_BUF);

            unsigned int packetsize = pack_msg1(buffer, &msg1);
            int bytes_send = send(socketfd, buffer, packetsize, 0);
            if (bytes_send < 0)
            {
                perror("ERROR in writting to socket.\n");
                exit(1);
            }

            print_time(); // printf time
            printf("[UL]:Sent %d of %u bytes of msg1: ue-id = %d\n", bytes_send, packetsize, msg1.ue_id);

            // MSG2 RRCSetup
            memset(buffer, 0, MAX_BUF);
            RRCSetup msg2;
            int bytes_recv = recv(socketfd, buffer, 1, 0);
            if (bytes_recv < 0)
            {
                perror("ERROR in reading from socket\n");
                exit(1);
            }
            unpack_msg2(buffer, &msg2);

            print_time(); // printf time
            printf("[DL]:Received %d bytes: \n\t\t\tmsg.type: %u\n",
                   bytes_recv, msg2.type2);

            if (msg2.type2 == 2)
            {
                memset(buffer, 0, MAX_BUF);
                RRCSetupComplete msg3 = {3};
                packetsize = pack_msg3(buffer, &msg3);
                bytes_send = send(socketfd, buffer, packetsize, 0);
                if (bytes_send < 0)
                {
                    perror("ERROR in writting to socket.\n");
                    exit(1);
                }
                print_time(); // printf time
                printf("[UL]:Sent %d of %u bytes of msg3.\n",
                       bytes_send, packetsize);
            }
            printf("---------------------------------\n");
            // Increase i
            i++;
        }
        time_t end = time(NULL);
        /* compute remaining time to sleep and sleep */
        double dif = difftime(end, start);
        int seconds_to_sleep = (int)(interval - dif);
        if (seconds_to_sleep > 0)
        { /* don't sleep if we're already late */
            printf("Sleep at %s", ctime(&start));
            sleep(seconds_to_sleep);
            printf("Start at %s", ctime(&start));
        }
    }

    close(socketfd);
}

int random_ue_id(int minN, int maxN)
{
    return minN + rand() % (maxN + 1 - minN);
}
void print_time()
{
    time_t timer;
    char buffer[26];
    struct tm *tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%d-%m-%Y %H:%M:%S", tm_info);
    printf(YEL "[%s]" RESET, buffer);
}
