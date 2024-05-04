// GNU General Public License v3.0
// @knedl1k & @
/*
 * file receiver
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "packets.h"
#include "hash.h"
#include "CRC/checksum.h"


void managePacketStream(const int sockfd);
bool checkCRC(const myPacket_t packet);
bool sendOK(const int sockfd, const struct sockaddr_in);
bool sendERROR(const int sockfd, const struct sockaddr_in);
void error(const char *msg);

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(1);
    }

    const int local_port = atoi(argv[1]);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval timeout ={.tv_sec = 1, .tv_usec = 0};
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt RCV failed");
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt SND failed");

    //TODO move to separate function
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sockfd,(struct sockaddr *) &local_addr, sizeof(local_addr)) < 0)
        error("Error on binding");
    //TODO

    managePacketStream(sockfd);

    close(sockfd);
    return 0;
}

bool checkCRC(const myPacket_t packet){
    return crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc)) == packet.crc;
}

void managePacketStream(const int sockfd){
    FILE *fd = NULL;
    bool start_received = false;
    bool hash_check = false;

    myPacket_t packet;

    uint32_t file_size = 0;
    char *file_name = NULL;
    uint32_t received_size = 0;

    bool last_packet_num = 0;
    int last_packet_type = -1;
    int last_sent_packet = -1;

    uint8_t iter=0;

    struct sockaddr_in from;
    unsigned int from_len = sizeof(from);

    while(!hash_check){
        ssize_t recv_len = recvfrom(sockfd, &packet, sizeof(myPacket_t), //TODO handle the return value?
                                    0,(struct sockaddr *) &from, &from_len);
        ++iter;
        if(recv_len < 0){
            if(iter>=5) error("Error receiving data");
            else continue;
        }
        iter=0; //packet came thru, reset counter

        if(!checkCRC(packet)){ //CRC does not match
            if(! sendERROR(sockfd,from)) error("ERROR while sending an ERROR packet!\n");
            last_sent_packet=ERROR;
            continue;
        }

        if(packet.num == last_packet_num && packet.type == last_packet_type && last_sent_packet==OK){ //identical packet detected
            if(! sendOK(sockfd,from)) error("ERROR while sending an OK packet!\n");
            last_sent_packet=OK;
            continue;
        }

        last_sent_packet=-1;

        switch(packet.type){
            case NAME:;
                size_t file_name_len = strlen((char *) packet.dataPacket.data);
                file_name = malloc(file_name_len); //TODO handle the return value
                memcpy(file_name, packet.dataPacket.data, file_name_len);

                fd = fopen(file_name, "w"); //TODO handle the return value?
                fprintf(stderr, "INFO: file name - %s.\n", file_name);
                break;
            case SIZE:;
                size_t i = 0;
                for(;;){
                    if(packet.dataPacket.data[i] == '\0')
                        break;
                    file_size *= 10;
                    file_size += packet.dataPacket.data[i++] - '0';
                }
                fprintf(stderr, "INFO: file size - %d B.\n", file_size);
                break;
            case START:
                start_received = true;
                fprintf(stderr, "INFO: START received!\n");
                break;
            case DATA:
                //fprintf(stderr, "INFO: data packet received.\n");
                if(!fd || !start_received){
                    hash_check = true;
                    fprintf(stderr, "Error: data before START, exiting..\n");
                }

                if(received_size + sizeof(packet.dataPacket.data) < file_size){
                    fwrite(packet.dataPacket.data, 1, sizeof(packet.dataPacket.data),fd);
                    received_size += sizeof(packet.dataPacket.data);
                }else{
                    fwrite(packet.dataPacket.data, 1, file_size - received_size, fd);
                    received_size = file_size;
                }
                break;
            case END:
                fclose(fd);
                break;
            case SHA256SUM:;
                unsigned char sender_hash[SHA256_DIGEST_LENGTH];
                memcpy(sender_hash, packet.dataPacket.data, SHA256_DIGEST_LENGTH);
                unsigned char receiver_hash[SHA256_DIGEST_LENGTH];
                calculateHash(file_name, file_size, receiver_hash);

                fprintf(stderr,"%s\n",memcmp(sender_hash, receiver_hash, SHA256_DIGEST_LENGTH)
                    ? "INFO: SHA256sum mismatch!" : "INFO: SHA256sum OK!");

                hash_check = true;
                break;

            default:
                fprintf(stderr, "Error: instruction %d with data %s not implemented!\n",
                        packet.type, packet.dataPacket.data);
        }

        if(! sendOK(sockfd,from)) error("ERROR while sending an OK packet!\n");
        last_sent_packet=OK;
    }

    free(file_name);
}

bool sendOK(const int sockfd, const struct sockaddr_in from){
    myPacket_t packet={
            .type=OK,
            .crc=crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc))
    };
    return(SEND(sockfd,(char *) &packet, sizeof(myPacket_t), from) !=SENDTO_ERROR);
}

bool sendERROR(const int sockfd, const struct sockaddr_in from){
    myPacket_t packet={
            .type=ERROR,
            .crc=crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc))
    };
    return(SEND(sockfd,(char *) &packet, sizeof(myPacket_t), from) !=SENDTO_ERROR);
}

void error(const char *msg){
    fprintf(stderr,"%s",msg);
    exit(100);
}
