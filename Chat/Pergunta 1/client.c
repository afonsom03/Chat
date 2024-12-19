#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include "memory.h"
#include "debug.h"
#include "common.h"
#include "client_opt.h"

struct sockaddr_in get_server_addr(const char *ip, unsigned short port);
void request_status(int udp_socket);
void request_on(int udp_socket, int device);
void request_off(int udp_socket, int device);

int main(int argc, char *argv[])
{
    /* Estrutura gerada pelo utilitario gengetopt */
    struct gengetopt_args_info args;

    /* Processa os parametros da linha de comando */
    if (cmdline_parser(argc, argv, &args) != 0)
    {
        exit(ERR_ARGS);
    }
    if (args.port_arg < 1 || args.port_arg > USHRT_MAX)
    {
        fprintf(stderr, "Invalid Port range. Accept values: 1-%hu\n", USHRT_MAX);
        exit(1);
    }
    if (strcmp(args.request_arg, "status") != 0 && !args.device_given)
    {
        fprintf(stderr, "A device number should be given 'for' on and 'off' requests\n");
        exit(2);
    }
    int udp_client_socket;
    if ((udp_client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        ERROR(21, "Can't create udp_client_socket (IPv4)");
    }
    struct sockaddr_in server_addr = get_server_addr(args.ip_arg, args.port_arg);

    if (connect(udp_client_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1)
    {
        ERROR(31, "Can't connect to server");
    }

    if (strcmp(args.request_arg, "status") == 0)
    {
        request_status(udp_client_socket);
    }
    else if (strcmp(args.request_arg, "on") == 0)
    {

        request_on(udp_client_socket, args.device_arg);
    }
    else if (strcmp(args.request_arg, "off") == 0)
    {

        request_off(udp_client_socket, args.device_arg);
    }

    close(udp_client_socket);
    cmdline_parser_free(&args);

    return 0;
}

void request_status(int udp_socket)
{

    uint8_t request = STATUS_REQUEST;

        if (send(udp_socket, &request, sizeof(request), 0) == -1)
            ERROR(24, "Can't sendto server");

    uint16_t status;

    if (recv(udp_socket, &status, sizeof(status), 0) == -1)
        ERROR(25, "Can't recvfrom server");

    status = ntohs(status);

    uint16_t mask = 1;
    //Podia-se fazer mas tinhamos de fazer as 8 linhas de comando, com o far fazemos apenas 3, mais otimizado computacionalmente.
    // printf("Portão da garagem: %s\n", status & mask != 0 ? "Ligado/Aberto" : "Desligado/Fechado");
    // mask <<= 1;
    // printf("Ilumincação hall entrada: %s\n", status & mask != 0 ? "Ligado/Aberto" : "Desligado/Fechado");
    // mask <<= 1;
    // printf("Ilumincação da saça: %s\n", status & mask != 0 ? "Ligado/Aberto" : "Desligado/Fechado");


    for(int i = 0; i < NUM_DEVICES; i++){
        printf("%s: %s\n", devices_names[i], (status & mask) != 0 ? "Ligado/Aberto" : "Desligado/Fechado");
        mask <<= 1;
    }
}

void request_on(int udp_socket, int device)
{

    uint8_t request[2] = {ON_REQUEST, (uint8_t) device};

    if (send(udp_socket, request, 2, 0) == -1)
        ERROR(24, "Can't sendto server");

    uint8_t reply;

    if (recv(udp_socket, &reply, sizeof(reply), 0) == -1)
        ERROR(25, "Can't recvfrom server");

    switch(reply)
    {
        case UNCHANGED_REPLY:
            printf("Comando \"Ligar/Abrir\"- \"%s\" ignorado\n", devices_names[device-1]);
            break;
        case CHANGED_REPLY:
            printf("Comando \"Ligar/Abrir\"- \"%s\" executado com sucesso\n", devices_names[device-1]);
            break;
        case INVALID_DEVICE_REPLY:
            printf("Dispositivo #%d desconhecido\n", device);
            break;
    }
}

void request_off(int udp_socket, int device)
{

    uint8_t request[2] = {OFF_REQUEST, (uint8_t) device};

    if (send(udp_socket, request, 2, 0) == -1)
        ERROR(24, "Can't sendto server");

    uint8_t reply;

    if (recv(udp_socket, &reply, sizeof(reply), 0) == -1)
        ERROR(25, "Can't recvfrom server");

    switch(reply)
    {
        case UNCHANGED_REPLY:
            printf("Comando \"Desligar/Fechar\"- \"%s\" ignorado\n", devices_names[device-1]);
            break;
        case CHANGED_REPLY:
            printf("Comando \"Desligar/Fechar\"- \"%s\" executado com sucesso\n", devices_names[device-1]);
            break;
        case INVALID_DEVICE_REPLY:
            printf("Dispositivo #%d desconhecido\n", device);
            break;
    }
}

struct sockaddr_in get_server_addr(const char *ip, unsigned short port)
{

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(struct sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    switch (inet_pton(AF_INET, ip, &server_address.sin_addr.s_addr))
    {
    case 0:
        fprintf(stderr, "[%s@%d] ERROR - Cannot convert IP address (IPv4): Invalid Network Address\n",
                __FILE__, __LINE__);
        exit(22);
    case -1:
        ERROR(22, "Cannot convert IP address (IPv4)");
    }
    return server_address;
}
