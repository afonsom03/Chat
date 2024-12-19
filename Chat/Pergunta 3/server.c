/**
 * @file: server.c
 * @date: 2024-12-19 13h08:56 
 * @author: Patricio R. Domingues
 */
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
 
#include "debug.h"
#include "common.h"
#include "server_opt.h"
 
int main(int argc, char *argv[]) {
    /* Estrutura gerada pelo utilitario gengetopt */
    struct gengetopt_args_info args;
 
    /* Processa os parametros da linha de comando */
    if (cmdline_parser(argc, argv, &args) != 0) {
        exit(ERR_ARGS);
    }
 
    int seed;
    if( args.seed_given ){
        seed = args.seed_arg;
        if ( (seed < 0) || (seed > 65535) ){
            fprintf(stderr,"[SERVER] Seed out of allow values [0,65535]\n");
            exit(1);
        }
    }else{
        seed = 1;   // default value
    }
    srand(seed);
 
    // UDP IPv4: cria socket
    int udp_server_socket;
    if ((udp_server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        ERROR(31, "Can't create udp_server_socket (IPv4)");
 
    // UDP IPv4: bind a IPv4/porto 
    struct sockaddr_in udp_server_endpoint;
    memset(&udp_server_endpoint, 0, sizeof(struct sockaddr_in));
    udp_server_endpoint.sin_family = AF_INET;
    udp_server_endpoint.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_server_endpoint.sin_port = htons(args.port_arg);
    if (bind(udp_server_socket, (struct sockaddr *) &udp_server_endpoint, sizeof(struct sockaddr_in)) == -1)
        ERROR(32, "Can't bind @udp_server_endpoint info");
        
    // UDP IPv4: variáveis auxiliares para sendto() / recvfrom()
    socklen_t udp_client_endpoint_length = sizeof(struct sockaddr_in);
    struct sockaddr_in udp_client_endpoint;
    ssize_t udp_read_bytes, udp_sent_bytes;
    char recv_data;
    
    while(1){
        // UDP IPv4: "recvfrom" do cliente (bloqueante)
        printf("à espera de dados do cliente... "); fflush(stdout);
        if ((udp_read_bytes = recvfrom(udp_server_socket, &recv_data, sizeof(recv_data), 
               0, (struct sockaddr *) &udp_client_endpoint, &udp_client_endpoint_length)) == -1){
            ERROR(34, "Can't recvfrom client");
        }
        printf("[SERVER] ok. (%d bytes received)\n", (int)udp_read_bytes);
 
        uint8_t num_to_clnt; 
        if( recv_data == 'N' ){
            num_to_clnt = (rand() % 50) + 1;
        }else if(recv_data == 'E'){
            num_to_clnt = (rand() % 12) + 1;
        }else{
            fprintf(stderr,"[ERROR] Wrong client request '%c'\n", recv_data);
            continue;
        }
 
        printf("[SERVER] client request '%c'\n", recv_data);
 
        printf("[SERVER] sending '%d' to client\n", num_to_clnt);
 
        if ((udp_sent_bytes = sendto(udp_server_socket, &num_to_clnt, sizeof(num_to_clnt), 0,
                (struct sockaddr *) &udp_client_endpoint, udp_client_endpoint_length)) == -1){
            ERROR(35, "Cannot sendto client");
        }
        printf("[SERVER] ok.  (%d bytes sent)\n", (int)udp_sent_bytes);
 
    }//while(1)
    
    cmdline_parser_free(&args);
    // liberta recurso: socket UDP IPv4
    if (close(udp_server_socket) == -1)
        ERROR(33, "Can't close udp_server_socket (IPv4)");
 
    return 0;
}