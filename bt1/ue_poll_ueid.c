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
#define LOOPCOUNT  30 // Seconds for UE to send msg



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "bt1.h"

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
void *messaging(void *sockfd);

// message
/*
 * ------------------------------
 * Main function
 * ------------------------------
 */
int main(int argc, char *argv[])
{

    // Set up socket for connection
    printf("Start simulating...\n");
    srand((int)time(0));

    SOCKET fds[10] = {0};
    SOCKET sockfd;
    int ue_count = 10;

    // connect ue_count socketfd to gNB
    for (int i = 0; i < ue_count; i++)
    {
        sockfd = get_connect_socket();
        fds[i] = sockfd;
    }

    // Multi-threading for sending and receiving simutaneously
    pthread_t thread_id;

    for (int i = 0; i < ue_count; i++)
    {
        pthread_create(&thread_id, NULL, messaging, (void *)(fds + i));
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

void *messaging(void *sockfd)
{
    SOCKET *socketfd;
    socketfd = (SOCKET *)sockfd;
    int ue_id = random_ue_id(100, 999);
    
    for (int j = 0; j < LOOPCOUNT; j++)
    {
        double interval = 1;       // 1 second
        time_t start = time(NULL); // Start
        unsigned char buffer[MAX_BUF];
        RRCSetupRequest msg1 = {1, ue_id, 3};

        int i = 0;
        while (i < 10) // Number of message1 per second
        {
            // MSG1 RRCSetupRequest
            memset(buffer, 0, MAX_BUF);

            unsigned int packetsize = pack_msg1(buffer, &msg1);
            int bytes_send = send(*socketfd, buffer, packetsize, 0);
            if (bytes_send < 0)
            {
                perror("ERROR in writting to socket.\n");
            }

            print_time(); // printf time
            printf("[UL]:Sent %d of %u bytes of msg1:\n" MAG "\tmsg.type: %u\n\tmsg.ue-id: %u\n \tmsg.cause: %u\n" RESET, 
            bytes_send, packetsize, msg1.type1, msg1.ue_id, msg1.cause);


            // MSG2 RRCSetup
            memset(buffer, 0, MAX_BUF);
            RRCSetup msg2;
            int bytes_recv = recv(*socketfd, buffer, 1, 0);
            if (bytes_recv <= 0)
            {
                perror("ERROR in reading from socket\n");
            }
            unpack_msg2(buffer, &msg2);

            if (msg2.type2 == 2)
            {
                print_time(); // printf time
                printf("[DL]:Received %d bytes: \n" MAG "\tmsg.type: %u\n" RESET,
                   bytes_recv, msg2.type2);
                
                // discard rate
                if (is_discard(0.1))
                {
                    printf(RED "Error: Can not send msg3.\n" RESET);
                }
                else
                {
                    memset(buffer, 0, MAX_BUF);
                    RRCSetupComplete msg3 = {3, ue_id};
                    packetsize = pack_msg3(buffer, &msg3);
                    bytes_send = send(*socketfd, buffer, packetsize, 0);
                    if (bytes_send < 0)
                    {
                        perror("ERROR in writting to socket.\n");
                    }
                    print_time(); // printf time
                    printf("[UL]:Sent %d of %u bytes of msg3:\n" MAG "\tmsg.type: %u\n\tmsg.ue-id: %u\n" RESET,
                        bytes_send, packetsize, msg3.type3, msg3.ue_id);
                }
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
            printf("End at %s", ctime(&start));
            sleep(seconds_to_sleep);
        }
    }

    close(*socketfd);
}

int random_ue_id(int minN, int maxN)
{
    return minN + rand() % (maxN + 1 - minN);
}

int is_discard(float fail_rate)
{
    int fail_var = 1;
    return (fail_var == random_ue_id(1, (int) 1/fail_rate));
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