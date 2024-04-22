// GNU General Public License v3.0
// @knedl1k & @
/*
 * File sender
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "hash.h"
#include "packets.h"


void error(const char *msg);
FILE* fileOpen(const char *fn);
void socksInit(struct sockaddr_in *server_addr, const char* server_ip, int server_port,
                struct sockaddr_in *client_addr, int client_port, int sockfd);
void sendHeader(const char *file_name, int sockfd, struct sockaddr_in server_addr, size_t file_size);
bool sendFile(FILE *fd, int sockfd, struct sockaddr_in server_addr);
bool endFileTransfer(int sockfd, struct sockaddr_in server_addr);
size_t fileLength(const char *file_name);
bool sendString(const int sockfd, const struct sockaddr_in server_addr, myPacket_t packet, unsigned char* str, const size_t len);
bool hashResolve(const int sockfd, const struct sockaddr_in server_addr, const char *file_name, size_t file_size);


int main(int argc, char *argv[]){
    if(argc<5){
        fprintf(stderr,"Usage: %s <server_ip> <server_port> <file_name> <client_port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip=argv[1];
    const int server_port=atoi(argv[2]);
    const char *file_name=argv[3];
    const int client_port=atoi(argv[4]);

    size_t file_size=fileLength(file_name);

    int sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd<0)
        error("Error opening socket");

    struct sockaddr_in server_addr, client_addr;

    socksInit(&server_addr, server_ip, server_port, &client_addr, client_port, sockfd);
    FILE *fd=fileOpen(file_name);

    sendHeader(file_name, sockfd, server_addr, file_size);

    if(! sendFile(fd, sockfd, server_addr))
        error("Error has occured in transfer!\n");

    if(! endFileTransfer(sockfd, server_addr))
        error("Error while closing the transmission!\n");

    if(! hashResolve(sockfd, server_addr, file_name, file_size))
        error("Error while sending a SHA256sum!\n");

    /*
    myPacket_t packet;
    socklen_t client_len=sizeof(client_addr);

    ssize_t recv_len=recvfrom(sockfd, &packet, sizeof(myPacket_t), //TODO handle the return value?
                                  0,(struct sockaddr *) &client_addr,&client_len);
    printf("code:%d\n",packet.type);
    */
    fclose(fd);
    close(sockfd);
    return 0;
}

bool sendString(const int sockfd, const struct sockaddr_in server_addr, myPacket_t packet, unsigned char* str, const size_t len){
    bool ret=true;
    size_t pos=0;

    while(pos < len && ret){
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        for(size_t i=0; i<PACKET_MAX_LEN-PACKET_OFFSET && i<len; ++i){
            packet.dataPacket.data[i]=str[i+pos];
        }
        pos += PACKET_MAX_LEN - PACKET_OFFSET;

        if(SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr) == SENDTO_ERROR)
            ret=false;
    }
    return ret;
}

bool sendFile(FILE *fd, const int sockfd, const struct sockaddr_in server_addr){
    myPacket_t packet={.type=DATA, .crc=0};
    bool ret=true;
    while(! feof(fd) && ret){
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        fread(packet.dataPacket.data, PACKET_MAX_LEN-PACKET_OFFSET, 1, fd);

        if(SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr) == SENDTO_ERROR)
            ret=false;
    }
    return ret;
}

bool endFileTransfer(const int sockfd, const struct sockaddr_in server_addr){
    myPacket_t packet={.type=END, .crc=0};
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    return SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr) != SENDTO_ERROR;
}

void sendHeader(const char *file_name, const int sockfd, const struct sockaddr_in server_addr, const size_t file_size){
    myPacket_t packet;

    packet.type=NAME;
    packet.crc=0;
    memcpy(packet.dataPacket.data, file_name, PACKET_MAX_LEN-4);
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

    packet.type=SIZE;
    packet.crc=0;
    sprintf((char*)packet.dataPacket.data, "%ld",file_size);
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

    packet.type=START;
    packet.crc=0;
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
    SEND(sockfd, (char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?
}

void socksInit(struct sockaddr_in *server_addr, const char* server_ip, const int server_port,
                struct sockaddr_in *client_addr, const int client_port, const int sockfd){
    memset(server_addr, '\0', sizeof(*server_addr));
    server_addr->sin_family=AF_INET;
    server_addr->sin_port=htons(server_port);

    if(inet_aton(server_ip, &server_addr->sin_addr) == 0)
        error("Invalid server IP address");

    memset(client_addr, '\0', sizeof(*client_addr));
    client_addr->sin_family=AF_INET;
    client_addr->sin_addr.s_addr=INADDR_ANY;
    client_addr->sin_port=htons(client_port);

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

size_t fileLength(const char *file_name){
    FILE *fl=fileOpen(file_name);
    fseek(fl, 0L, SEEK_END);
    size_t file_size=ftell(fl);
    fseek(fl, 0L, SEEK_SET);
    fclose(fl);
    return file_size;
}

bool hashResolve(const int sockfd, const struct sockaddr_in server_addr, const char *file_name, size_t file_size){
    unsigned char sha256_hash[SHA256_DIGEST_LENGTH];
    calculateHash(file_name, file_size, sha256_hash);
    myPacket_t packet={.type=SHA256SUM, .crc=0};
    return sendString(sockfd, server_addr,packet, sha256_hash, SHA256_DIGEST_LENGTH);
}