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
#define GNB_BACKLOG 100
#define MAX_BUF 2048
#define PORT "5001" // Port to listen()
#define UE_LIST_SIZE 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
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

in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);

    return (((struct sockaddr_in6*)sa)->sin6_port);
}


// return socket_listen
SOCKET get_listener_socket(void);
// Handle recv and send message
void msg_handle(unsigned char buffer[], SOCKET sender_fd, int *ue_count, int bytes_recv);
// Add new file descriptor to the pollset
void add_to_pfds(struct pollfd *pfds[], SOCKET newfd, int *fd_count, int *fd_size);
// Remove an index form pollset
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count);
// Check if ue_id in the list and add it if not
int in_ue_list(uint32_t ue_id, uint32_t arr[]);
// Add to ue list
void add_to_ue_list(uint32_t ue_id, uint32_t arr[], int *ue_count);
// Del from ue_list after RRC_Succ
void del_from_ue_list(uint32_t ue_id, uint32_t arr[], int *ue_count, int ue_list_size);
// Print ue_list
void print_arr(uint32_t arr[], int n);

/*
 * ------------------------------
 * Main function
 * ------------------------------
 */

// KPI etc.
int RRC_Att_total = 0, RRC_ReAtt = 0, RRC_Succ = 0;

uint32_t ue_list[UE_LIST_SIZE];
struct timeval t1[UE_LIST_SIZE], t2[UE_LIST_SIZE];
double KPI_msec = 0;
int num_element = 0;

int main(int argc, char *argv[])
{

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

    memset(&ue_list, 0, sizeof(ue_list));
    int ue_count = 0;

    // Main loop
    while (1)
    {
        // Check number of socket fd which event occurred
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            perror("poll");
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
                        // IF not add to UE list
                        add_to_pfds(&pfds, socket_client, &fd_count, &fd_size);

                        // Get client addr and change to presentation (struct to string)

                        inet_ntop(client_address.ss_family, (struct sockaddr *)&client_address,
                                  ip_addr, INET6_ADDRSTRLEN);
                        printf(GRN "Poll(): new connection from %s on socket %d (PORT: %d)\n" RESET, ip_addr,
                               socket_client, get_in_port((struct sockaddr *)&client_address));
                    }
                }
                else
                {
                    // If not socket_listener but socket_client
                    unsigned char buffer[MAX_BUF];
                    memset(buffer, 0, MAX_BUF);
                    SOCKET sender_fd = pfds[i].fd;
                    int bytes_recv = recv(sender_fd, buffer, 1, 0);

                    // Check error
                    if (bytes_recv <= 0)
                    {
                        if (bytes_recv == 0)
                        {
                            printf(GRN "pollserver: socket %d hung up\n" RESET, sender_fd);
                        }
                        else
                        {
                            perror("ERROR in reading from socket");
                        }
                        close(sender_fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        // recv msg1 then send msg2; recv msg3
                        msg_handle(buffer, sender_fd, &ue_count, bytes_recv);
                    }

                } // END handle data from socket_client
            }     // END got ready-to-read socket from poll()
        }         // END looping through sock fd
        printf("RRC_Succ/RRC_ReAtt/RRC_Att_total: %d / %d / %d\nRRC_SR = %.2lf%%\n", RRC_Succ,
               RRC_ReAtt, RRC_Att_total, ((double)RRC_Succ / (RRC_Att_total)) * 100);
        printf(GRN "Timer KPI: %f ms.\n" RESET, KPI_msec);

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
    // memset(&hints, 0, sizeof(hints));
    // hints.ai_family = AF_INET;
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;

    // if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
    // {
    //     fprintf(stderr, "getaddrinfo %s\n", gai_strerror(status));
    //     return -1;
    // }

    // Create Socket
    printf("Creating socket....\n");
    SOCKET socket_listener;
    socket_listener = socket(AF_INET, SOCK_STREAM, 0);
    //socket_listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // Check valid
    if (!ISVALIDSOCKET(socket_listener))
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        return -1;
    }

    // SO_REUSEPORT
    int optval = 1;
    setsockopt(socket_listener, SOL_SOCKET, SO_REUSEPORT, &optval,sizeof(optval));

    // // SO_REUSEADDR
    // int yes = 1;

    // // lose the "Address already in use" error message
    // if (setsockopt(socket_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    // {
    //     perror("Setsockopt() failed");
    //     exit(1);
    // }

    // bind host address
    printf("Binding address....\n");
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(5001);
    if (bind (socket_listener, (struct sockaddr*)&server_addr,sizeof(server_addr)))
    // if (bind(socket_listener, res->ai_addr, res->ai_addrlen))
    {
        fprintf(stderr, "bind() failed. (%d)\n", errno);
        close(socket_listener);
        return -1;
    }

    // Free addrinfo after binding socket
    //freeaddrinfo(res);

    // Listening
    printf("Listening...\n");
    if (listen(socket_listener, GNB_BACKLOG) == -1)
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
    // set events to check if ready-to-read
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

int in_ue_list(uint32_t ue_id, uint32_t arr[])
{
    int check = 0;
    for (int i = 0; i < UE_LIST_SIZE; i++)
    {
        if (ue_id == arr[i])
        {
            check = 1;
        }
    }
    return check;
}

void add_to_ue_list(uint32_t ue_id, uint32_t arr[], int *ue_count)
{
    for (int i = 0; i <= *ue_count; i++)
    {
        if (arr[i] == 0 || arr[i] == -1)
        {
            gettimeofday(&t1[i], NULL);
            arr[i] = ue_id;
            (*ue_count)++;
            break;
        }
    }
}

void del_from_ue_list(uint32_t ue_id, uint32_t arr[], int *ue_count, int ue_list_size)
{
    for (int i = 0; i < ue_list_size; i++)
    {
        if (arr[i] == ue_id)
        {
            // Handle KPI_timer
            gettimeofday(&t2[i], NULL);
            double elapsedTime;
            elapsedTime = (t2[i].tv_sec - t1[i].tv_sec) * 1000.0;    // sec to ms
            elapsedTime += (t2[i].tv_usec - t1[i].tv_usec) / 1000.0; // us to ms
            KPI_msec += (elapsedTime - KPI_msec) / num_element;

            // Handle del element
            printf(YEL "DEL from UE_list: ue_id (%u)\nUE_count before: %d\n", ue_id, *ue_count);
            arr[i] = -1;
            (*ue_count)--;
            print_arr(arr, *ue_count);
            printf(YEL "UE_count after: %d\n" RESET, *ue_count);
        }
    }
}

void print_arr(uint32_t arr[], int n)
{
    printf(YEL "ue-list: ");
    for (int i = 0; i < n; i++)
    {
        printf("%d ", arr[i]);
    }
    printf("\n" RESET);
}

void msg_handle(unsigned char buffer[], SOCKET sender_fd, int *ue_count, int bytes_recv)
{
    // bytes_recv > 0
    if (buffer[0] == 1)
    {
        RRC_Att_total++;

        int bytes_recv2 = recv(sender_fd, buffer + 1, 6, 0);
        if (bytes_recv2 < 0)
        {
            perror("ERROR in reading from socket");
        }

        RRCSetupRequest msg1;
        unpack_msg1(buffer, &msg1);
        printf("[DL]:Received %d bytes: \n" MAG "\tmsg.type: %u\n\tmsg.ue-id: %u\n \tmsg.cause: %u\n" RESET, bytes_recv + bytes_recv2, msg1.type1, msg1.ue_id, msg1.cause);
        // Check if UE in the UE_list
        if (in_ue_list(msg1.ue_id, ue_list) == 1)
        {
            RRC_ReAtt += 1;

            printf(YEL "UE-id: %u ReAttempt/" RESET, msg1.ue_id);
            print_arr(ue_list, *ue_count);
        }
        else
        {
            add_to_ue_list(msg1.ue_id, ue_list, ue_count);
            printf(YEL "ADD NEW -- UE-id: %u " RESET, msg1.ue_id);
            print_arr(ue_list, *ue_count);
            num_element++; // Increament number in KPI_timer
        }
        // send msg2 RRCSetup
        RRCSetup msg2 = {2};
        unsigned int packetsize = pack_msg2(buffer, &msg2);
        int bytes_send = send(sender_fd, buffer, packetsize, 0);
        if (bytes_send < 0)
        {
            perror("ERROR in writting to socket");
        }

        printf("[UL]:Sent %d of %u bytes of msg2.\n" MAG "\tmsg.type: %u\n" RESET, bytes_send, packetsize, msg2.type2);
        // memset(&msg1 , 0 , sizeof(msg1));
    }

    else if (buffer[0] == 3)
    {

        RRC_Succ++;
        int bytes_recv3 = recv(sender_fd, buffer + 1, 4, 0);
        if (bytes_recv3 < 0)
        {
            perror("ERROR in reading from socket");
        }

        // Unpack msg3 and delete ue_id from ue_list
        RRCSetupComplete msg3;
        unpack_msg3(buffer, &msg3);

        printf("[DL]:Received %d bytes: \n" MAG "\tmsg.type: %u\n\tmsg.ue-id: %u\n" RESET,
               bytes_recv + bytes_recv3, msg3.type3, msg3.ue_id);
        del_from_ue_list(msg3.ue_id, ue_list, ue_count, UE_LIST_SIZE);
        printf("---------------------------------\n");
        // memset(&msg1 , 0 , sizeof(msg1));
    }
}