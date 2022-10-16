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
pthread_t tid[5];
sigset_t fSigSet;

#define ISVALIDSOCKET(s) ((s) >= 0)
#define SOCKET int
#define PORT "8080" // Port to listen()
#define GNB_BACKLOG 100
#define MAX_BUF 1024

void *serverThreadFunc(void *arg);
void *sfn_counter_Thread(void *msec);
void *paging_Thread(void *sockfd);

SOCKET get_listener_socket(void);
int msleep(long msec);
void print_current_time_with_ms (void);


/*
** MAIN function
*/
int main(int argc, char *argv[])
{
    int msec = 10; // Time per SFN

    pthread_create(&tid[0], NULL, sfn_counter_Thread, (void*)&msec);
    pthread_create(&tid[1], NULL, serverThreadFunc, NULL);
    
    pthread_exit(NULL);
    return 0;
}

void *serverThreadFunc(void *arg)
{
    printf("Main thread...\n");
    SOCKET socket_client;
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    char ip_addr[INET6_ADDRSTRLEN];

    SOCKET socket_listener = get_listener_socket();
    if (socket_listener == -1)
    {
        fprintf(stderr, "error getting listening socket\n");
        exit(EXIT_FAILURE);
    }

    socket_client = accept(socket_listener, (struct sockaddr *) & client_address, &client_len);

    if(!ISVALIDSOCKET(socket_client))
    {
        fprintf(stderr, "Accept() failed (%d)\n", errno);        
    }
    
    unsigned int number1;
    scanf("%u",&number1);
    int bytes_sent = send(socketfd, &number1, 4, 0);
    if (bytes_sent <= 0)
    {
         perror("ERROR in sending to socket");
         exit(EXIT_FAILURE);
    }
    SFN = number1;

}


/*
** SFN thread
*/
void *sfn_counter_Thread(void *msec)
{
    int status;
    int *interval = (int *)msec;
    while (1)
    {

        if (SFN == 1024)
        {
            SFN = 0;
        }

        SFN++;
        usleep(10000);
    }
}


/*
** Create sockfd from socket(), bind(), listen()
*/
SOCKET get_listener_socket(void)
{
    printf("Creating socket....\n");
    struct addrinfo hints, *res;
    int status;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(status));
        return -1;
    }

    SOCKET socket_listener;
    socket_listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (!ISVALIDSOCKET(socket_listener))
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        return -1;
    }

    // Set sockopt to handle errno 98
    int opt = 1;
    if (setsockopt(socket_listener, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(socket_listener, res->ai_addr, res->ai_addrlen))
    {
        fprintf(stderr, "bind() failed. (%d)\n", errno);
        close(socket_listener);
        return -1;
    }

    freeaddrinfo(res);

    if (listen(socket_listener, GNB_BACKLOG) == -1)
    {
        fprintf(stderr, "listen() failed. (%d)\n", errno);
        return -1;
    }

    return socket_listener;
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

