/**
 * @file: client.c
 * @date: 2024-12-19 13h35:56 
 * @author: Patricio R. Domingues
 */
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
 
#include "memory.h"
#include "debug.h"
#include "common.h"
#include "client_opt.h"
 
int main(int argc, char *argv[]) {
    /* Estrutura gerada pelo utilitario gengetopt */
    struct gengetopt_args_info args;
 
    /* Processa os parametros da linha de comando */
    if (cmdline_parser(argc, argv, &args) != 0) {
        exit(ERR_ARGS);
    }
 
    int udp_client_socket;  
    if ((udp_client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        ERROR(21, "Can't create udp_client_socket (IPv4)");
 
    socklen_t udp_server_endpoint_length = sizeof(struct sockaddr_in);  
    struct sockaddr_in udp_server_endpoint;
    memset(&udp_server_endpoint, 0, sizeof(struct sockaddr_in));
    udp_server_endpoint.sin_family = AF_INET;   
    switch (inet_pton(AF_INET, args.ip_arg, &udp_server_endpoint.sin_addr.s_addr)) {
        case 0:
            fprintf(stderr, 
            "[%s@%d] ERROR - Cannot convert IP address (IPv4): Invalid Network Address\n",
                    __FILE__, __LINE__);
            exit(22);
        case -1:
            ERROR(22, "Cannot convert IP address (IPv4)");
    }
    udp_server_endpoint.sin_port = htons(args.port_arg);
 
 
    // UDP IPv4: variáveis auxiliares para sendto() / recvfrom()
    ssize_t udp_read_bytes, udp_sent_bytes;
    char send_to_server;
    int8_t num_from_server;
 
    int euroTostoes_regular[5];
    int num_regular = 0;
    int euroTostoes_stars[2];
    int num_sends = 0;
    int keep_number = 0;
 
    while(num_regular < 5){
 
        printf("[CLNT] Sending request to server\n");
        send_to_server = 'N';
        num_sends++;
        if ((udp_sent_bytes = sendto(udp_client_socket, &send_to_server, sizeof(send_to_server), 0,
            (struct sockaddr *) &udp_server_endpoint, udp_server_endpoint_length)) == -1){
            ERROR(24, "Can't sendto server");
        }
        printf("[CLNT] num_sends=%d -- ok.(%d bytes sent)\n", num_sends, (int)udp_sent_bytes);
 
        printf("[CLNT] Waiting for server answer...\n");
        if ((udp_read_bytes = recvfrom(udp_client_socket, &num_from_server, sizeof(num_from_server), 0, 
            (struct sockaddr *) &udp_server_endpoint, &udp_server_endpoint_length)) == -1){
            ERROR(25, "Can't recvfrom server");
        }
        printf("[CLNT] Received '%d' (%d bytes received)\n", num_from_server, (int)udp_read_bytes); 
 
        keep_number = 1;
        for(int i=0; i<num_regular; i++){
            if( euroTostoes_regular[i] == num_from_server ){
                // Repeated number
                printf("[INFO] number '%d' already in our eurotostoes bet\n", num_from_server);
                keep_number = 0;
                break;
            }
        }//for
        if( keep_number ){
            euroTostoes_regular[num_regular] = num_from_server;
            num_regular++;
        }
    }//while(1)
 
    // Display regular vector
    for(int i=0; i<num_regular; i++){
        printf("regular[%d] = %d\n", i, euroTostoes_regular[i]);
    }
    cmdline_parser_free(&args);
    if (close(udp_client_socket) == -1)
        ERROR(23, "Can't close udp_client_socket (IPv4)");
 
    return 0;
}