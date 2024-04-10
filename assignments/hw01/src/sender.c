// GNU General Public License v3.0
// @knedl1k & @
/*
 * File sender
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "packets.h"

#define SEND(sockfd,send_buffer,len,server_addr) \
                (sendto(sockfd, send_buffer, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)))

void error(const char *msg);
FILE* fileOpen(char *fn);
void socksInit(struct sockaddr_in *server_addr, const char* server_ip, const int server_port,
                struct sockaddr_in *client_addr, const int client_port, const int sockfd);
void sendHeader(char *filename, int sockfd, struct sockaddr_in server_addr);


int main(int argc, char *argv[]){
    if(argc < 5){
        fprintf(stderr,"Usage: %s <server_ip> <server_port> <filename> <client_port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip=argv[1];
    const int server_port=atoi(argv[2]);
    char *filename=argv[3];
    const int client_port=atoi(argv[4]);

    int sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("Error opening socket");

    struct sockaddr_in server_addr, client_addr;
    socksInit(&server_addr, server_ip, server_port, &client_addr, client_port, sockfd);

    FILE *fd=fileOpen(filename);

    sendHeader(filename, sockfd, server_addr);

    myPacket_t packet={.type=DATA, .crc=0};
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    while(! feof(fd)){
         fread(packet.dataPacket.data, PACKET_MAX_LEN-4, 1, fd);
         SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr);

    }

    packet.type=END;
    packet.crc=0;
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr);

    fclose(fd);
    close(sockfd);
    return 0;
}

void sendHeader(char *filename, int sockfd, struct sockaddr_in server_addr){
    myPacket_t packet;

    packet.type=NAME;
    packet.crc=0;
    memcpy(packet.dataPacket.data, filename, PACKET_MAX_LEN-4);
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr);

    packet.type=SIZE;
    packet.crc=0;
    FILE *fd=fileOpen(filename);
    fseek(fd, 0L, SEEK_END);
    sprintf((char*)packet.dataPacket.data, "%ld",ftell(fd));
    fseek(fd, 0L, SEEK_SET);
    fclose(fd);
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr);

    packet.type=START;
    packet.crc=0;
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr);
}

void socksInit(struct sockaddr_in *server_addr, const char* server_ip, const int server_port,
                struct sockaddr_in *client_addr, const int client_port, const int sockfd){
    memset(server_addr, '\0', sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(server_port);

    if(inet_aton(server_ip, &server_addr->sin_addr) == 0)
        perror("Invalid server IP address");

    memset(client_addr, '\0', sizeof(*client_addr));
    client_addr->sin_family = AF_INET;
    client_addr->sin_addr.s_addr = INADDR_ANY;
    client_addr->sin_port = htons(client_port);

    if(bind(sockfd, (struct sockaddr *) client_addr, sizeof(*client_addr)) < 0)
        perror("Error on binding");
}

FILE* fileOpen(char *fn){
    FILE *fd=fopen(fn, "rb");
    if(fd==NULL)
        error("Error opening file");
    return fd;
}

void error(const char *msg){
    perror(msg);
    exit(1);
}
