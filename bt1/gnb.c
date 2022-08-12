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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serialize.h"

int main(int argc, char *argv[])
{
    // Get address info
    printf("Get address info....\n");
    struct addrinfo hints;
    int status;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *res;
    if ((status = getaddrinfo(NULL, "5001", &hints, &res)) != 0)
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
    // SO_REUSEADDR
    int yes = 1;

    // lose the "Address already in use" error message
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    // bind host address
    printf("Binding address....\n");
    if (bind(socketfd, res->ai_addr, res->ai_addrlen))
    {
        fprintf(stderr, "bind() failed. (%d)\n", errno);
        return 1;
    }

    // Free addrinfo after binding socket
    freeaddrinfo(res);

    // Listening
    printf("Listening...\n");
    if (listen(socketfd, 5))
    {
        fprintf(stderr, "listen() failed. (%d)\n", errno);
        return 1;
    }

    // Waiting
    printf("Waiting for connection...\n");
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);

    SOCKET socket_client = accept(socketfd, (struct sockaddr *)&client_address, &client_len);

    if (!ISVALIDSOCKET(socket_client))
    {
        fprintf(stderr, "Accept() failed. (%d)\n", errno);
        return 1;
    }

    // Get client addr and chang to presentation (struct to string)
    char ip_addr[INET6_ADDRSTRLEN];
    inet_ntop(client_address.ss_family, (struct sockaddr *)&client_address, ip_addr, INET6_ADDRSTRLEN);
    printf("Client Connected from %s\n", ip_addr);

    // Start communication
    printf("Start simulating...\n");

    unsigned char buffer[MAX_BUF];
    RRCSetupRequest msg1;
    int RRC_Att = 0, RRC_Succ = 0;
    int i = 0;
    while (i < 100)
    {
        memset(buffer, 0, MAX_BUF);
        int bytes_recv = recv(socket_client, buffer, 1, 0);
        if (bytes_recv < 0)
        {
            perror("ERROR in reading from socket\n");
            return 1;
        }

        if (buffer[0] == 1)
        {
            int bytes_recv2 = recv(socket_client, buffer + 1, 6, 0);
            if (bytes_recv2 < 0)
            {
                perror("ERROR in reading from socket\n");
                return 1;
            }
            unpack_msg1(buffer, &msg1);
            RRC_Att++;
            RRCSetup msg2;
            msg2.type2 = 2;
            unsigned int packetsize = pack_msg2(buffer, &msg2);
            int bytes_send = send(socket_client, buffer, packetsize, 0);
            if (bytes_send < 0)
            {
                perror("ERROR in writting to socket.\n");
                return 1;
            }
            printf("[DL]:Received %d bytes: \n\tmsg.type: %u\n\tmsg.ue-id: %u\n\tmsg.cause: %u\n",
                   bytes_recv + bytes_recv2, msg1.type1, msg1.ue_id, msg1.cause);
            printf("[UL]:Sent %d of %u bytes of msg2.\n", bytes_send, packetsize);
            // memset(&msg1 , 0 , sizeof(msg1));
        }

        if (buffer[0] == 3)
        {
            RRC_Succ++;
            RRCSetupComplete msg3;
            unpack_msg3(buffer, &msg3);

            printf("[DL]:Received %d bytes: \n\tmsg.type: %u\n",
                   bytes_recv, msg3.type3);
            printf("---------------------------------\n");
            // memset(&msg1 , 0 , sizeof(msg1));
        }

        i++;
    }
    printf("RRC_SR = %d : %d\n", RRC_Att, RRC_Succ);

    close(socket_client);
    close(socketfd);

    return 0;
}