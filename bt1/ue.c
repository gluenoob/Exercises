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
#define PORT 5001

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
void print_time();

// Main function
int main(int argc, char *argv[])
{
    // Get address info
    printf("Get address info....\n");
    struct addrinfo hints;
    int status;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res;
    if ((status = getaddrinfo("127.0.0.1", PORT, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(status));
        return 1;
    }

    // Create Socket
    printf("Creating socket....\n");
    SOCKET socketfd;
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // Check valid
    if (!ISVALIDSOCKET(socketfd))
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        return 1;
    }

    // Connect
    if (connect(socketfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        close(socketfd);
        perror("client: connect");
        return 1;
    }
    // Free addrinfo after binding socket
    freeaddrinfo(res);

    // Start communication
    printf("Start simulating...\n");

    unsigned char buffer[MAX_BUF];
    RRCSetupRequest msg1 = {1, 123, 3};
    int RRC_Att = 0, RRC_Succ = 0;
    int i = 0;
    while (i < 10)
    {
        // MSG1 RRCSetupRequest
        memset(buffer, 0, MAX_BUF);

        unsigned int packetsize = pack_msg1(buffer, &msg1);
        int bytes_send = send(socketfd, buffer, packetsize, 0);
        if (bytes_send < 0)
        {
            perror("ERROR in writting to socket.\n");
            return 1;
        }
        print_time();
        printf("[UL]:Sent %d of %u bytes of msg1.\n", bytes_send, packetsize);

        // MSG2 RRCSetup
        memset(buffer, 0, MAX_BUF);
        RRCSetup msg2;
        int bytes_recv = recv(socketfd, buffer, 1, 0);
        if (bytes_recv < 0)
        {
            perror("ERROR in reading from socket\n");
            return 1;
        }
        unpack_msg2(buffer, &msg2);
        print_time();
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
                return 1;
            }
            print_time();
            printf("[UL]"
                   ":Sent %d of %u bytes of msg3.\n",
                   bytes_send, packetsize);
        }
        printf("---------------------------------\n");
        // Increase i
        i++;
    }

    close(socketfd);
    return 0;
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