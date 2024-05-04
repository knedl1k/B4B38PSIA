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


void error(const char *msg);
FILE* fileOpen(const char *fn);
void socksInit(struct sockaddr_in *server_addr, const char* server_ip, int server_port);
void sendHeader(const char *file_name, int sockfd, struct sockaddr_in server_addr, size_t file_size);
bool sendFile(FILE *fd, int sockfd, struct sockaddr_in server_addr);
bool endFileTransfer(int sockfd, struct sockaddr_in server_addr);
size_t fileLength(const char *file_name);
bool sendString(const int sockfd, const struct sockaddr_in server_addr, myPacket_t packet, unsigned char* str, const size_t len);
bool hashResolve(const int sockfd, const struct sockaddr_in server_addr, const char *file_name, size_t file_size);


int main(int argc, char *argv[]){
    if(argc<4){
        fprintf(stderr,"Usage: %s <server_IP> <server_port> <file_name>\n", argv[0]);
        exit(1);
    }
    const char *server_ip=argv[1];
    const int server_port=atoi(argv[2]);
    const char *file_name=argv[3];

    const size_t file_size=fileLength(file_name);

    int sockfd;
    if((sockfd=socket(AF_INET, SOCK_DGRAM, 0))<0)
        error("Error opening send socket");
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt RCV failed");
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt SND failed");

    struct sockaddr_in server_addr;
    socksInit(&server_addr, server_ip, server_port);

    FILE *fd=fileOpen(file_name);

    sendHeader(file_name, sockfd, server_addr, file_size);

    if(! sendFile(fd, sockfd, server_addr))
        error("Error has occured in transfer!\n");

    if(! endFileTransfer(sockfd, server_addr))
        error("Error while closing the transmission!\n");
    //printf("file was transfered\n");
    if(! hashResolve(sockfd, server_addr, file_name, file_size))
        error("Error while sending a SHA256sum!\n");


    fclose(fd);
    close(sockfd);
    return 0;
}

bool sendString(const int sockfd, const struct sockaddr_in server_addr, myPacket_t packet, unsigned char* str, const size_t len){
    bool ret=true;
    size_t pos=0;
    unsigned char buffer[sizeof(packet.dataPacket.data)];

    myPacket_t tmp=packet;
    socklen_t server_len=sizeof(server_addr);
    packet.num = 0;

    while(pos < len && ret){
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        for(size_t i=0; i<PACKET_MAX_LEN-PACKET_OFFSET && i<len; ++i){
            buffer[i]=str[i+pos];
        }
        pos += PACKET_MAX_LEN - PACKET_OFFSET;
        uint8_t iter=0;
        for(;;){
            memcpy(packet.dataPacket.data, buffer, sizeof(packet.dataPacket.data));
            packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));
            packet.type=tmp.type;

            if(SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr) == SENDTO_ERROR){
                ret=false;
                break;
            }

        if(recvfrom(sockfd, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &server_addr, &server_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");
        }

            if(packet.type == OK) break;

            if(++iter>=5) error("Error CRC: hash\n");
        }
     packet.num++;
    }
   
    return ret;
}

bool sendFile(FILE *fd, const int sockfd, const struct sockaddr_in server_addr){
    myPacket_t packet;
    bool ret=true;
    socklen_t server_len=sizeof(server_addr);
    unsigned char buffer[sizeof(packet.dataPacket.data)];
    packet.num =0;
    while(! feof(fd) && ret){
        uint8_t iter=0;
        memset(buffer, 0, sizeof(buffer));
        fread(buffer, PACKET_MAX_LEN-PACKET_OFFSET, 1, fd);
        for(;;){
            packet.type=DATA;
            memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
            memcpy(packet.dataPacket.data, buffer, sizeof(buffer));

            packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));

            if(SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr) == SENDTO_ERROR){ //TODO handle return value?
                ret=false;
                break;
            }

            if(recvfrom(sockfd, &packet, sizeof(myPacket_t),
            0,(struct sockaddr *) &server_addr, &server_len)==-1){
                if(errno == EAGAIN && errno == EWOULDBLOCK)
                    fprintf(stderr,"INFO: did not receive ACK!\n");

            }

            if(packet.type == OK) break;

            if(++iter>=5) error("Error CRC: sendFile\n");
            //usleep(100);
        }
    packet.num++;
    }
    
    return ret;
}

bool endFileTransfer(const int sockfd, const struct sockaddr_in server_addr){
    bool ret=true;
    myPacket_t packet;
    uint8_t iter=0;
    socklen_t server_len=sizeof(server_addr);
    packet.num = 0;
    for(;;){
        packet.type = END;
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        packet.crc = crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc));
        if(SEND(sockfd,(char *) &packet, sizeof(myPacket_t), server_addr)==SENDTO_ERROR){ //TODO handle return value?
            ret=false;
            break;
        }
        if(recvfrom(sockfd, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &server_addr, &server_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");

        }

        if(packet.type == OK) break;

        if(++iter >= 5) error("Error CRC: END");
    }


    return ret;
}

void sendHeader(const char *file_name, const int sockfd, const struct sockaddr_in server_addr, const size_t file_size){
    myPacket_t packet;
    uint8_t iter=0;
    socklen_t server_len=sizeof(server_addr);
    packet.num = 0;
    

    for(;;){
        packet.type = NAME;
        memcpy(packet.dataPacket.data, file_name, PACKET_MAX_LEN - PACKET_OFFSET);
        packet.crc = crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc));
        SEND(sockfd,(char *) &packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

        if(recvfrom(sockfd, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &server_addr, &server_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");

        }

        if(packet.type == OK) break;

        if(++iter >= 5) error("Error CRC: NAME");
    
    }
    packet.num++;
    iter=0;
    for(;;){
        packet.type=SIZE;
        sprintf((char*)packet.dataPacket.data, "%ld",file_size);
        packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));
        SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

        if(recvfrom(sockfd, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &server_addr, &server_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");

        }
        if(packet.type == OK) break;

        if(++iter>=5) error("Error CRC: SIZE");
    
    }
    packet.num++;

    iter=0;
    for(;;){
        packet.type=START;
        memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));
        packet.crc=crc_32((unsigned char*)&packet, sizeof(packet)-sizeof(packet.crc));
        SEND(sockfd,(char*)&packet, sizeof(myPacket_t), server_addr); //TODO handle return value?

        if(recvfrom(sockfd, &packet, sizeof(myPacket_t),
           0,(struct sockaddr *) &server_addr, &server_len)==-1){
            if(errno == EAGAIN && errno == EWOULDBLOCK)
                fprintf(stderr,"INFO: did not receive ACK!\n");

        }

        if(packet.type == OK) break;

        if(++iter>=5) error("Error CRC: START");
        
    }
}

void socksInit(struct sockaddr_in *server_addr, const char* server_ip, const int server_port){
    server_addr->sin_family=AF_INET;
    server_addr->sin_port=htons(server_port);
    if(inet_aton(server_ip, &server_addr->sin_addr) == 0)
        error("Invalid server IP address");
}

FILE* fileOpen(const char *fn){
    FILE *fd=fopen(fn, "rb");
    if(fd==NULL)
        error("Error opening file");
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

bool hashResolve(const int sockfd, const struct sockaddr_in server_addr, const char *file_name, size_t file_size){
    unsigned char sha256_hash[SHA256_DIGEST_LENGTH];
    calculateHash(file_name, file_size, sha256_hash);
    myPacket_t packet={.type=SHA256SUM};
    return sendString(sockfd, server_addr,packet, sha256_hash, SHA256_DIGEST_LENGTH);
}