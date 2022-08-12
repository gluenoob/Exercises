#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define SOCKET int
#define MAX_BUF 1024
#define PORT "5001" // Port to listen()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

// return socket_listen
SOCKET get_listener_socket(void);
// Add new file descriptor to the pollset
void add_to_pfds(struct pollfd *pfds[], SOCKET newfd, int *fd_count, int *fd_size);
// Remove an index form pollset
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count);

/*
 * ------------------------------
 * Main function
 * ------------------------------
 */
int main(int argc, char *argv[])
{
    // // Time for main to run

    // time_t start = time(NULL); // Start
    // time_t interval = 20;      // Time wait
    // time_t endwait = start + interval;

    unsigned char buffer[MAX_BUF];
    // Set up for socket client
    printf("Setting up connection...\n");
    SOCKET socket_client;                   // New Accept() socket fd
    struct sockaddr_storage client_address; // Client Address
    socklen_t client_len = sizeof(client_address);
    char ip_addr[INET6_ADDRSTRLEN];

    // Create listen socket
    SOCKET socket_listener = get_listener_socket();
    if (socket_listener == -1)
    {
        fprintf(stderr, "error getting listening socket\n");
        return 1;
    }

    // Setup poll() struct array for 5 connection
    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof(*pfds) * fd_size);

    // Add socket_listener to the array
    pfds[0].fd = socket_listener;
    pfds[0].events = POLLIN;
    fd_count = 1;

    // Start communication
    printf("Start Simulating...\n");
    int RRC_Att = 0, RRC_Succ = 0;
    // Main loop
    // while (start < endwait) // loop with time
    while (1)
    {
        // Check number of socket fd which event occurred
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1)
        {
            perror("poll\n");
            exit(1);
        }

        // Run through socket fd that event occurred
        for (int i = 0; i < fd_count; i++)
        {

            // Check if socket ready to read
            if (pfds[i].revents & POLLIN)
            {
                // If yes
                if (pfds[i].fd == socket_listener)
                {
                    // If sockfd is socket_listener, accept to create new socket_client
                    socket_client = accept(socket_listener, (struct sockaddr *)&client_address, &client_len);

                    // Check if socket_client error or not
                    if (!ISVALIDSOCKET(socket_client))
                    {
                        fprintf(stderr, "Accept() failed. (%d)\n", errno);
                    }
                    else
                    {
                        add_to_pfds(&pfds, socket_client, &fd_count, &fd_size);

                        // Get client addr and change to presentation (struct to string)

                        inet_ntop(client_address.ss_family, (struct sockaddr *)&client_address,
                                  ip_addr, INET6_ADDRSTRLEN);
                        printf(GRN "Poll(): new connection from %s on socket %d\n" RESET, ip_addr,
                               socket_client);
                    }
                }
                else
                {
                    // If not socket_listener but socket_client

                    // int total = 0;
                    //  loop for recv and send msg
                    // while (1)
                    //{
                    RRCSetupRequest msg1;
                    memset(buffer, 0, MAX_BUF);
                    int bytes_recv = recv(pfds[i].fd, buffer, 1, 0);

                    int sender_fd = pfds[i].fd;

                    if (bytes_recv <= 0)
                    {
                        if (bytes_recv == 0)
                        {
                            printf(GRN "pollserver: socket %d hung up\n" RESET, sender_fd);
                            // break;
                        }
                        else
                        {
                            perror("ERROR in reading from socket\n");
                            // break;
                        }
                        close(pfds[i].fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        // bytes_recv > 0
                        if (buffer[0] == 1)
                        {
                            int bytes_recv2 = recv(sender_fd, buffer + 1, 6, 0);
                            if (bytes_recv2 < 0)
                            {
                                perror("ERROR in reading from socket\n");
                            }
                            unpack_msg1(buffer, &msg1);
                            RRC_Att++;

                            // send msg2 RRCSetup
                            RRCSetup msg2 = {2};
                            unsigned int packetsize = pack_msg2(buffer, &msg2);
                            int bytes_send = send(pfds[i].fd, buffer, packetsize, 0);
                            if (bytes_send < 0)
                            {
                                perror("ERROR in writting to socket.\n");
                            }
                            printf("[DL]:Received %d bytes: \n\tmsg.type: %u\n" MAG "\tmsg.ue-id: %u\n" RESET "\tmsg.cause: %u\n",
                                   bytes_recv + bytes_recv2, msg1.type1, msg1.ue_id, msg1.cause);
                            printf("[UL]:Sent %d of %u bytes of msg2.\n", bytes_send, packetsize);
                            // memset(&msg1 , 0 , sizeof(msg1));
                        }

                        else if (buffer[0] == 3)
                        {
                            RRC_Succ++;
                            RRCSetupComplete msg3;
                            unpack_msg3(buffer, &msg3);

                            printf("[DL]:Received %d bytes: \n\tmsg.type: %u\n",
                                   bytes_recv, msg3.type3);
                            printf("---------------------------------\n");
                            // memset(&msg1 , 0 , sizeof(msg1));
                        }
                    }

                    // total++;
                    //}

                    // close(pfds[i].fd); // CLOSE socket pfds[i].fd

                    // del_from_pfds(pfds, i, &fd_count);
                } // END handle data from socket_client

            } // END got ready-to-read from poll()
        }     // END looping through sock fd
        printf("RRC_SR = %d : %d\n", RRC_Succ, RRC_Att);

        // // Update time left to run
        // start = time(NULL);
    } // END while(1) loop

    close(socket_listener);

    return 0;
}

SOCKET get_listener_socket(void)
{
    // Get address info
    printf("Get address info....\n");
    struct addrinfo hints, *res;
    int status;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(status));
        exit(1);
    }

    // Create Socket
    printf("Creating socket....\n");
    SOCKET socket_listener;
    socket_listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // Check valid
    if (!ISVALIDSOCKET(socket_listener))
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        exit(1);
    }

    // SO_REUSEADDR
    int yes = 1;

    // lose the "Address already in use" error message
    if (setsockopt(socket_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        perror("Setsockopt() failed");
        exit(1);
    }

    // bind host address
    printf("Binding address....\n");
    if (bind(socket_listener, res->ai_addr, res->ai_addrlen))
    {
        fprintf(stderr, "bind() failed. (%d)\n", errno);
        close(socket_listener);
        return 1;
    }

    // Free addrinfo after binding socket
    freeaddrinfo(res);

    // Listening
    printf("Listening...\n");
    if (listen(socket_listener, 10) == -1)
    {
        fprintf(stderr, "listen() failed. (%d)\n", errno);
        return -1;
    }

    return socket_listener; // Return the main socket fd
}

void add_to_pfds(struct pollfd *pfds[], SOCKET newfd, int *fd_count, int *fd_size)
{
    // If pdfs full, x2 the size
    if (*fd_count == *fd_size)
    {
        *fd_size *= 2;

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    // Add newfd
    (*pfds)[*fd_count].fd = newfd;
    // Check if ready-to-read
    (*pfds)[*fd_count].events = POLLIN;

    (*fd_count)++;
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy last element to the struct need tobe deleted
    pfds[i] = pfds[*fd_count - 1];

    // Reduce the count by 1
    (*fd_count)--;
}