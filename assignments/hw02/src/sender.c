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
#include <errno.h>

#include "hash.h"
#include "packets.h"
#include "CRC/checksum.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

void error(const char *msg);
FILE* fileOpen(const char *fn);
void socksInit(struct sockaddr_in *server_addr, const char* server_ip, int server_port);
void sendHeader(const char *file_name, int sockfd, struct sockaddr_in server_addr, size_t file_size, struct sockaddr_in listen_addr, const int sockfd2);
bool sendFile(FILE *fd, int sockfd, struct sockaddr_in server_addr, struct sockaddr_in listen_addr, const int sockfd2);
size_t fileLength(const char *file_name);
bool sendString(const int sockfd, const struct sockaddr_in server_addr, myPacket_t packet, unsigned char* str, const size_t len, struct sockaddr_in listen_addr, const int sockfd2);
bool hashResolve(const int sockfd, const struct sockaddr_in server_addr, const char *file_name, size_t file_size, struct sockaddr_in listen_addr, const int sockfd2);

bool checkCRC(const myPacket_t packet){
    return crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc)) == packet.crc;
}

int main(int argc, char *argv[]){
    if(argc<4){
        fprintf(stderr,"Usage: %s <server_IP> <server_port> <file_name>\n", argv[0]);
        exit(1);
    }
    /*
    const char *server_ip=argv[1];
    const int server_port=atoi(argv[2]);
    */
    const char *server_ip="127.0.0.1";
    const int server_port=14000;

    const char *file_name=argv[3];

    const size_t file_size=fileLength(file_name);

    int sockfd;
    if((sockfd=socket(AF_INET, SOCK_DGRAM, 0))<0)
        error("Error opening send socket\n");
    struct timeval timeout={.tv_sec=1, .tv_usec=0};
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt RCV failed\n");
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt SND failed\n");

    struct sockaddr_in server_addr;
    socksInit(&server_addr, server_ip, server_port);

    int sockfd2;
    if((sockfd2=socket(AF_INET, SOCK_DGRAM, 0))<0)
        error("Error opening send socket\n");

    if(setsockopt(sockfd2, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt RCV failed\n");
    if(setsockopt(sockfd2, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt SND failed\n");

    struct sockaddr_in listen_addr;
    listen_addr.sin_family=AF_INET;
    listen_addr.sin_port=htons(15001);
    //listen_addr.sin_addr.s_addr=htons("127.0.0.1");
    inet_aton(server_ip, &listen_addr.sin_addr);
    if(bind(sockfd2,(struct sockaddr *) &listen_addr, sizeof(listen_addr)) < 0)
        error("Error on binding");


    FILE *fd=fileOpen(file_name);

    sendHeader(file_name, sockfd, server_addr, file_size, listen_addr, sockfd2);

    if(! sendFile(fd, sockfd, server_addr, listen_addr, sockfd2))
        error("Error has occured in transfer!\n");

    myPacket_t end={.type=END, .num=100};
    if( ! sendString(sockfd, server_addr, end, (unsigned char *)"\0", sizeof("\0"), listen_addr, sockfd2))
        error("Error while sending an END!\n");

    //printf("file was transfered\n");
    if(! hashResolve(sockfd, server_addr, file_name, file_size, listen_addr, sockfd2))
        error("Error while sending a SHA256sum!\n");

    fprintf(stderr,"INFO: Transfer complete!\n");
    fclose(fd);
    close(sockfd);
    return 0;
}

bool sendString(const int sockfd, const struct sockaddr_in server_addr, myPacket_t packet, unsigned char* str,
                const size_t len, struct sockaddr_in listen_addr, const int sockfd2){
    bool ret=true;
    size_t pos=0;
    unsigned char buffer[sizeof(packet.dataPacket.data)];

    myPacket_t tmp=packet;
    //socklen_t server_len=sizeof(listen_addr);
    unsigned char nums=packet.num;
    //printf("Sending a HASH!\n");
    struct sockaddr_in from;
    unsigned int from_len=sizeof(from);

    while(pos < len && ret){
        memset(buffer, 0, sizeof(buffer));
        for(size_t i=0; i<PACKET_MAX_LEN-PACKET_OFFSET && i<len; ++i)
            buffer[i]=str[i+pos];

        pos += PACKET_MAX_LEN - PACKET_OFFSET;
        uint8_t iter=0;
        for(;;){
            memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
            memcpy(packet.dataPacket.data, buffer, sizeof(packet.dataPacket.data));
            packet.type=tmp.type;
            packet.num=nums;
            packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));
            //printf("crc:\n%u\n---\n",packet.crc);

            if(SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr) == SENDTO_ERROR){
                ret=false;
                break;
            }

            if(recvfrom(sockfd2, &packet, sizeof(myPacket_t),
               0,(struct sockaddr *) &from, &from_len)==-1){
                if(errno == EAGAIN && errno == EWOULDBLOCK)
                    fprintf(stderr,"INFO: did not receive ACK!\n");
            }

            if(checkCRC(packet) && packet.type == OK) break;

            if(++iter>=MAX_ITER) error("Error CRC: string\n");
        }
        ++nums;
    }

    return ret;
}

bool sendFile(FILE *fd, const int sockfd, const struct sockaddr_in server_addr, struct sockaddr_in listen_addr, const int sockfd2){
    myPacket_t packet;
    bool ret=true;
    socklen_t server_len=sizeof(listen_addr);
    unsigned char buffer[sizeof(packet.dataPacket.data)];
    struct sockaddr_in from;
    size_t nums=4;
    //packet.num=3;
    while(! feof(fd) && ret){
        uint8_t iter=0;
        memset(buffer, 0, sizeof(packet.dataPacket.data));
        size_t bytes_read=fread(buffer, 1, sizeof(packet.dataPacket.data),  fd); //1
        //printf("bytes:%zu\n",bytes_read);
        for(;;){
            packet.type=DATA;
            packet.num=nums;
            memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
            memcpy(packet.dataPacket.data, buffer, bytes_read);

            packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));
            //printf("read:\n%s\n---\n",buffer);
            //printf("crc:\n%u\n---\n",packet.crc);

            if(SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr) == SENDTO_ERROR){
                ret=false;
                break;
            }

            if(recvfrom(sockfd2, &packet, sizeof(myPacket_t),
            0,(struct sockaddr *) &from, &server_len)==-1){
                if(errno == EAGAIN && errno == EWOULDBLOCK)
                    fprintf(stderr,"INFO: did not receive ACK!\n");
            }

            if(checkCRC(packet) && packet.type == OK) break;

            if(++iter>=MAX_ITER) error("Error CRC: sendFile\n");
        }
        ++nums;
    }

    return ret;
}

void sendHeader(const char *file_name, const int sockfd, const struct sockaddr_in server_addr, const size_t file_size,
                struct sockaddr_in listen_addr, const int sockfd2){
    myPacket_t packet;
    uint8_t iter=0;
    socklen_t server_len=sizeof(listen_addr);
    unsigned char nums=1;

    struct sockaddr_in from;
    unsigned int from_len=sizeof(from);

    for(;;){
        packet.type=NAME;
        packet.num=nums;
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        memcpy(packet.dataPacket.data, file_name, PACKET_MAX_LEN - PACKET_OFFSET);
        packet.crc=crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc));
        SEND(sockfd,(char *) &packet, sizeof(myPacket_t), server_addr); //TODO handle return value?
        if(recvfrom(sockfd2, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &from, &from_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");
        }
        if(checkCRC(packet) && packet.type == OK) break;

        if(++iter >= MAX_ITER) error("Error CRC: NAME\n");
    }
    ++nums;

    iter=0;
    for(;;){
        packet.type=SIZE;
        packet.num=nums;
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        sprintf((char*)packet.dataPacket.data, "%ld",file_size);
        packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));
        SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

        if(recvfrom(sockfd2, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &from, &server_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");
        }
        if(checkCRC(packet) && packet.type == OK) break;

        if(++iter>=MAX_ITER) error("Error CRC: SIZE\n");
    }
    ++nums;

    iter=0;
    for(;;){
        packet.type=START;
        packet.num=nums;
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));
        SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

        if(recvfrom(sockfd2, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &from, &server_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");
        }
        if(checkCRC(packet) && packet.type == OK) break;

        if(++iter>=MAX_ITER) error("Error CRC: START\n");
    }
}

void socksInit(struct sockaddr_in *server_addr, const char* server_ip, const int server_port){
    server_addr->sin_family=AF_INET;
    server_addr->sin_port=htons(server_port);
    if(inet_aton(server_ip, &server_addr->sin_addr) == 0)
        error("Invalid server IP address\n");
}

FILE* fileOpen(const char *fn){
    FILE *fd=fopen(fn, "rb");
    if(fd==NULL)
        error("Error opening file\n");
    return fd;
}

void error(const char *msg){
    fprintf(stderr,"%s",msg);
    exit(4);
}

size_t fileLength(const char *file_name){
    FILE *fl=fileOpen(file_name);
    fseek(fl, 0L, SEEK_END);
    size_t file_size=ftell(fl);
    fseek(fl, 0L, SEEK_SET);
    fclose(fl);
    return file_size;
}

bool hashResolve(const int sockfd, const struct sockaddr_in server_addr, const char *file_name, size_t file_size,
                 struct sockaddr_in listen_addr, const int sockfd2){
    unsigned char sha256_hash[SHA256_DIGEST_LENGTH];
    calculateHash(file_name, file_size, sha256_hash);
    myPacket_t packet={.type=SHA256SUM, .num=0};
    return sendString(sockfd, server_addr,packet, sha256_hash, SHA256_DIGEST_LENGTH, listen_addr, sockfd2);
}