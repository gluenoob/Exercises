#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <inttypes.h>
#include "bt2.h"

int SFN = 0;

#define ISVALIDSOCKET(s) ((s) >= 0)
#define SOCKET int
#define MAX_BUF 1024
#define SERVERADDR "127.0.0.1"
#define PORT "8080"

void *receiving(void *arg);
void *sfn_counter_Thread(void *msec);

int msleep(long msec);
SOCKET get_connect_socket();
void print_current_time_with_ms (void);


int main(int argc, char *argv[])
{
    pthread_t thread_id;
    int msec = 10; // Time per SFN
    pthread_create(&thread_id, NULL, sfn_counter_Thread, (void*)&msec);

    pthread_create(&thread_id, NULL, receiving, NULL);
    
    
    pthread_exit(NULL);
    return 0;
}

/*
** Receive SFN from gNB
*/
void *receiving(void *arg)
{
    
    SOCKET socketfd = get_connect_socket();

    unsigned int number1;
    int bytes_recv = recv(socketfd, &number1, 4, 0);
    if (bytes_recv <= 0)
    {
         perror("ERROR in reading from socket");
         exit(EXIT_FAILURE);
    }
    
    SFN = number1;
}

/*
** SFN thread
*/
void *sfn_counter_Thread(void *msec)
{
    while (1)
    {
        int status;
        int *interval = (int *)msec;
        if (SFN == 1024)
        {
            SFN = 0;
        }

        SFN++;
        usleep(10000);
    }
}


/*
** Create sockfd from socket(), connect()
*/
SOCKET get_connect_socket()
{
    printf("Setting up connection...\n");
    struct sockaddr_storage client_address; // Client Address
    socklen_t client_len = sizeof(client_address);
    char ip_addr[INET6_ADDRSTRLEN];

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

    printf("Creating socket....\n");
    SOCKET socketfd;
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (!ISVALIDSOCKET(socketfd))
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        exit(1);
    }

    if (connect(socketfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        close(socketfd);
        perror("client: connect");
        exit(1);
    }

    freeaddrinfo(res);
    return socketfd;
}
/*
** Print Current time in milliseconds
*/
void print_current_time_with_ms (void)
{
    long            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }

    printf("Current time: %"PRIdMAX".%03ld seconds since the Epoch\n",
           (intmax_t)s, ms);
}

