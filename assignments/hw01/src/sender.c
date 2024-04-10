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

void error(const char *msg);
FILE* fileOpen(const char *fn);
void socksInit(struct sockaddr_in *server_addr, const char* server_ip, int server_port,
                struct sockaddr_in *client_addr, int client_port, int sockfd);
void sendHeader(const char *filename, int sockfd, struct sockaddr_in server_addr);
bool sendFile(FILE *fd, int sockfd, struct sockaddr_in server_addr);
bool endTransfer(int sockfd, struct sockaddr_in server_addr);

int main(int argc, char *argv[]){
    if(argc<5){
        fprintf(stderr,"Usage: %s <server_ip> <server_port> <filename> <client_port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip=argv[1];
    const int server_port=atoi(argv[2]);
    const char *filename=argv[3];
    const int client_port=atoi(argv[4]);

    int sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd<0)
        error("Error opening socket");

    struct sockaddr_in server_addr, client_addr;

    socksInit(&server_addr, server_ip, server_port, &client_addr, client_port, sockfd);
    FILE *fd=fileOpen(filename);

    sendHeader(filename, sockfd, server_addr);

    if(! sendFile(fd, sockfd, server_addr))
        error("Error has occured in transfer!\n");

    if(! endTransfer(sockfd, server_addr))
        error("Error while closing the transmission!\n");

    fclose(fd);
    close(sockfd);
    return 0;
}

bool sendFile(FILE *fd, const int sockfd, const struct sockaddr_in server_addr) {
    myPacket_t packet={.type=DATA, .crc=0};
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    bool ret=true;
    while(! feof(fd) && ret){
        fread(packet.dataPacket.data, PACKET_MAX_LEN-4, 1, fd);
        if(SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr) == SENDTO_ERROR)
            ret=false;
    }
    return ret;
}

bool endTransfer(const int sockfd, const struct sockaddr_in server_addr) {
    myPacket_t packet={.type=END, .crc=0};
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    return SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr) != SENDTO_ERROR;
}

void sendHeader(const char *filename, const int sockfd, const struct sockaddr_in server_addr){
    myPacket_t packet;

    packet.type=NAME;
    packet.crc=0;
    memcpy(packet.dataPacket.data, filename, PACKET_MAX_LEN-4);
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

    packet.type=SIZE;
    packet.crc=0;
    FILE *fd=fileOpen(filename);
    fseek(fd, 0L, SEEK_END);
    sprintf((char*)packet.dataPacket.data, "%ld",ftell(fd));
    fseek(fd, 0L, SEEK_SET);
    fclose(fd);
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

    packet.type=START;
    packet.crc=0;
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?
}

void socksInit(struct sockaddr_in *server_addr, const char* server_ip, const int server_port,
                struct sockaddr_in *client_addr, const int client_port, const int sockfd){
    memset(server_addr, '\0', sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(server_port);

    if(inet_aton(server_ip, &server_addr->sin_addr) == 0)
        error("Invalid server IP address");

    memset(client_addr, '\0', sizeof(*client_addr));
    client_addr->sin_family = AF_INET;
    client_addr->sin_addr.s_addr = INADDR_ANY;
    client_addr->sin_port = htons(client_port);

    if(bind(sockfd, (struct sockaddr *) client_addr, sizeof(*client_addr)) < 0)
        error("Error on binding");
}

FILE* fileOpen(const char *fn){
    FILE *fd=fopen(fn, "rb");
    if(fd==NULL)
        error("Error opening file");
    return fd;
}

void error(const char *msg){
    perror(msg);
    exit(100);
}
