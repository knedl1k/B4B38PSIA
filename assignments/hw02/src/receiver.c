// GNU General Public License v3.0
// @knedl1k & @
/*
 * File receiver
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

void managePacketStream(int sockfd, const struct sockaddr_in *client_addr);

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(1);
    }

    const int server_port=atoi(argv[1]);

    int sockfd;
    struct sockaddr_in server_addr, client_addr;

    sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
        error("Error opening socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(server_port);

    if(bind(sockfd,(struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        error("Error on binding");

    managePacketStream(sockfd, &client_addr);

    close(sockfd);
    return 0;
}

void managePacketStream(const int sockfd, const struct sockaddr_in *client_addr){
    FILE *file=NULL;
    bool start_received=false;
    bool stop_received=false;

    myPacket_t packet;
    uint32_t file_size=0;
    uint32_t received_size=0;
    socklen_t client_len=sizeof(client_addr);

    unsigned char received_hash[SHA256_DIGEST_LENGTH];
    unsigned char calculated_hash[SHA256_DIGEST_LENGTH];

    while(! stop_received){
        ssize_t recv_len=recvfrom(sockfd, &packet, sizeof(myPacket_t), //TODO handle the return value?
                                  0,(struct sockaddr *) &client_addr,&client_len);
        if(recv_len < 0)
            error("Error receiving data");

        switch(packet.type){
            case NAME:
                file=fopen((char *) packet.dataPacket.data, "w"); //TODO handle the return value?
                fprintf(stderr, "INFO: file name - %s\n", packet.dataPacket.data);
                break;
            case SIZE:;
                size_t i=0;
                for(;;){
                    if(packet.dataPacket.data[i] == '\0')
                        break;
                    file_size *= 10;
                    file_size += packet.dataPacket.data[i++] - '0';
                }
                fprintf(stderr, "INFO: file size - %d\n", file_size);
                break;
            case START:
                start_received=true;
                fprintf(stderr, "INFO: START received!\n");
                break;
            case DATA:
                if(!file || !start_received){
                    stop_received=true;
                    fprintf(stderr,"Error: data before START, exiting..\n");
                }
                //printf("n:%ld\n",recv_len - sizeof(short)*2);
                if(received_size + sizeof(packet.dataPacket.data) < file_size){
                    fwrite(packet.dataPacket.data, 1, sizeof(packet.dataPacket.data), file); //TODO handle the return value? recv_len - sizeof(short) * 2
                    received_size+=sizeof(packet.dataPacket.data);
                }else{
                    fwrite(packet.dataPacket.data, 1, file_size - received_size, file);
                    received_size=file_size;
                }
                break;
            case END:
                stop_received=true;
                break;
            default:
                fprintf(stderr, "Error: instruction %d with data %s not implemented!\n",
                        packet.type, packet.dataPacket.data);
        }
    }
    
    if(memcmp(received_hash, calculated_hash, SHA256_DIGEST_LENGTH) == 0) {
        printf("File received correctly\n");
    } else {
        printf("Error during file transmission\n");
    }

    fclose(file);
}

void error(const char *msg){
    perror(msg);
    exit(100);
}
