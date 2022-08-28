#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
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

#define UE_NUM 10
#define DISCARD_RATE 0.2
#define LOOP_COUNT 5
#define MSG_PER_LOOP 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "bt2.h"

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
int is_discard(float fail_rate);

// Return a connected socket to the gnb
SOCKET get_connect_socket();

// Send and receive msg function
void *sending(void *p_sockstruct);
void *sending_back(void *p_sockstruct);

typedef struct socket_thread
{
    SOCKET socketfd;
    int ue_id;
} t_socket;

// message
/*
 * ------------------------------
 * Main function
 * ------------------------------
 */
int main(int argc, char *argv[])
{
    // Initial setup
    printf("Setting up Connection...\n");
    srand((int)time(0));

    t_socket fds[UE_NUM];
    SOCKET sockfd;

    // connect ue_count socketfd to gNB
    for (int i = 0; i < UE_NUM; i++)
    {
        sockfd = get_connect_socket();
        printf("Socket client (%d) CONNECTED.\n", sockfd);
        fds[i].socketfd = sockfd;
        fds[i].ue_id = random_ue_id(1000, 9999);
    }

    // Set up for sending and receiving message

    // Multi-threading for sending and receiving simutaneously
    pthread_t thread_id;

    for (int i = 0; i < UE_NUM; i++)
    {
        pthread_create(&thread_id, NULL, sending, (void *)(fds + i));
        pthread_create(&thread_id, NULL, sending_back, (void *)(fds + i));
    }

    pthread_exit(NULL);

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

void *sending(void *p_sockstruct)
{
    t_socket *socketfd;
    socketfd = (t_socket *)p_sockstruct;

    for (int j = 0; j < LOOP_COUNT; j++)
    {
        double interval = 1;       // 1 second
        time_t start = time(NULL); // Start
        unsigned char buffer[MAX_BUF];
        RRCSetupRequest msg1 = {1, socketfd->ue_id, 3};

        int i = 0;
        while (i < MSG_PER_LOOP) // Number of message1 per second
        {
            // MSG1 RRCSetupRequest
            memset(buffer, 0, MAX_BUF);

            unsigned int packetsize = pack_msg1(buffer, &msg1);
            int bytes_send = send(socketfd->socketfd,
                                  buffer, packetsize, 0);
            if (bytes_send < 0)
            {
                perror("ERROR in writting to socket.");
                return NULL;
            }

            print_time(); // printf time
            printf("[UL]:Sent %d of %u bytes of msg1:\n" MAG "\tmsg.type: %u\n\tmsg.ue-id: %u\n \tmsg.cause: %u\n" RESET,
                   bytes_send, packetsize, msg1.type1, msg1.ue_id, msg1.cause);
            // Increase i
            i++;
        }
        time_t end = time(NULL);
        /* compute remaining time to sleep and sleep */
        double dif = difftime(end, start);
        int seconds_to_sleep = (int)(interval - dif);
        if (seconds_to_sleep > 0)
        { /* don't sleep if we're already late */
            printf("End at %s", ctime(&start));
            sleep(seconds_to_sleep);
        }
    }
}

void *sending_back(void *p_sockstruct)
{
    t_socket *socketfd;
    socketfd = (t_socket *)p_sockstruct;
    unsigned char buffer2[MAX_BUF];

    while (1) // Number of message1 per second
    {
        // MSG2 RRCSetup
        memset(buffer2, 0, MAX_BUF);
        RRCSetup msg2;
        int bytes_recv = recv(socketfd->socketfd, buffer2, 1, 0);
        if (bytes_recv <= 0)
        {
            perror("ERROR in reading from socket");
            return NULL;
        }
        unpack_msg2(buffer2, &msg2);

        if (msg2.type2 == 2)
        {
            print_time(); // printf time
            printf("[DL]:Received %d bytes: \n" MAG "\tmsg.type: %u\n" RESET, bytes_recv, msg2.type2);
            // discard rate
            if (is_discard(DISCARD_RATE))
            {
                printf(RED "Error: Can not send msg3.\n" RESET);
            }
            else
            {
                memset(buffer2, 0, MAX_BUF);
                RRCSetupComplete msg3 = {3, socketfd->ue_id};
                unsigned int packetsize = pack_msg3(buffer2, &msg3);
                int bytes_send = send(socketfd->socketfd, buffer2, packetsize, 0);
                if (bytes_send < 0)
                {
                    perror("ERROR in writting to socket");
                }
                print_time(); // printf time
                printf("[UL]:Sent %d of %u bytes of msg3:\n" MAG "\tmsg.type: %u\n\tmsg.ue-id: %u\n" RESET,
                       bytes_send, packetsize, msg3.type3, msg3.ue_id);
            }
        }
        printf("---------------------------------\n");
    }
}

int random_ue_id(int minN, int maxN)
{
    return minN + rand() % (maxN + 1 - minN);
}

int is_discard(float fail_rate)
{
    int fail_var = 1;
    return (fail_var == random_ue_id(1, (int)1 / fail_rate));
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
