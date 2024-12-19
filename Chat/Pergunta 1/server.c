#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "unistd.h"
#include "memory.h"
#include "debug.h"
#include "common.h"
#include "server_opt.h"

int bind_udp_server(unsigned short port);
struct sockaddr_in get_addr_for_all_interfaces(unsigned short port);
void handle_requests(int udp_socket, uint16_t status);
void handle_status_request(int udp_socket, uint16_t status, struct sockaddr *address, socklen_t address_length);
uint16_t handle_on_request(int udp_socket, uint16_t status, uint8_t device, struct sockaddr *address, socklen_t address_length);
uint16_t handle_off_request(int udp_socket, uint16_t status, uint8_t device, struct sockaddr *address, socklen_t address_length);

int main(int argc, char *argv[])
{
    /* Estrutura gerada pelo utilitario gengetopt */
    struct gengetopt_args_info args;

    /* Processa os parametros da linha de comando */
    if (cmdline_parser(argc, argv, &args) != 0)
    {
        exit(ERR_ARGS);
    }

    int udp_server_socket;
    if ((udp_server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        ERROR(21, "Can't create udp_client_socket (IPv4)");

    /* Registo local - bind */
    if (args.port_arg < 1 || args.port_arg > USHRT_MAX)
    {
        fprintf(stderr, "Invalid Port range. Accept values: 1-%hu\n", USHRT_MAX);
        exit(1);
    }
    if (args.status_arg < 0 || args.status_arg > 255)
    {
        fprintf(stderr, "Invalid Status value. %d\n", args.status_arg);
        exit(1);
    }

    int sockfd = bind_udp_server(args.port_arg);

    printf("Listening on port %d\n", args.port_arg);

    uint16_t status = (uint16_t)args.status_arg;

    cmdline_parser_free(&args);

    handle_requests(sockfd, status);

    // close(udp_server_socket);

    /* Liberta memória */

    close(udp_server_socket);

    return 0;
}

void handle_requests(int udp_socket, uint16_t status)
{
    socklen_t address_length = sizeof(struct sockaddr_in);
    struct sockaddr_in address;
    while (1)
    {

        uint8_t request[2] = {0, 0};

        if (recvfrom(udp_socket, request, 2, 0, (struct sockaddr *)&address, &address_length) == -1)
            ERROR(34, "Can't recvfrom client");

        switch (request[0])
        {
        case STATUS_REQUEST:
            handle_status_request(udp_socket, status, (struct sockaddr *)&address, address_length);
            break;
        case ON_REQUEST:
            status = handle_on_request(udp_socket, status, request[1], (struct sockaddr *)&address, address_length);
            break;
        case OFF_REQUEST:
            status = handle_off_request(udp_socket, status, request[1], (struct sockaddr *)&address, address_length);
            break;
        };
    }
}

void handle_status_request(int udp_socket, uint16_t status, struct sockaddr *address, socklen_t address_length)
{

    status = htons(status);
    if (sendto(udp_socket, &status, sizeof(status), 0, (struct sockaddr *)address, address_length) == -1)
        ERROR(35, "Can't sendto client");
}

uint16_t handle_on_request(int udp_socket, uint16_t status, uint8_t device, struct sockaddr *address, socklen_t address_length)
{

    uint8_t reply = INVALID_DEVICE_REPLY;
    if (device >= 1 && device <= NUM_DEVICES)
    {
        uint16_t mask = 1 << (device - 1);
        if ((status & mask) != 0)
        {
            reply = UNCHANGED_REPLY;
        }
        else
        {
            status = status | mask;
            reply = CHANGED_REPLY;
        }
    }

    if (sendto(udp_socket, &reply, sizeof(reply), 0, (struct sockaddr *)address, address_length) == -1)
        ERROR(35, "Can't sendto client");
    return status;
}

uint16_t handle_off_request(int udp_socket, uint16_t status, uint8_t device, struct sockaddr *address, socklen_t address_length)
{


    uint8_t reply = INVALID_DEVICE_REPLY;
    if (device >= 1 && device <= NUM_DEVICES)
    {
        uint16_t mask = 1 << (device - 1);
        if ((status & mask) == 0)
        {
            reply = UNCHANGED_REPLY;
        }
        else
        {
            status = status & ~mask;
            reply = CHANGED_REPLY;
        }
    }

    if (sendto(udp_socket, &reply, sizeof(reply), 0, (struct sockaddr *)address, address_length) == -1)
        ERROR(35, "Can't sendto client");
    return status;

}

int bind_udp_server(unsigned short port)
{
    int udp_server_socket;
    if ((udp_server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        ERROR(21, "Can't create udp_client_socket (IPv4)");

    struct sockaddr_in address = get_addr_for_all_interfaces(port);
    if (bind(udp_server_socket, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) == -1)
    {
        close(udp_server_socket);
        ERROR(32, "Can't bind @udp_server_endpoint info");
    }

    return udp_server_socket;
}
struct sockaddr_in get_addr_for_all_interfaces(unsigned short port)
{
    struct sockaddr_in udp_server_endpoint;
    memset(&udp_server_endpoint, 0, sizeof(struct sockaddr_in));
    udp_server_endpoint.sin_family = AF_INET;                // se for ipv6 é ADD_INET6
    udp_server_endpoint.sin_addr.s_addr = htonl(INADDR_ANY); // Todas as interfaces de rede
    udp_server_endpoint.sin_port = htons(port);
    return udp_server_endpoint;
}
