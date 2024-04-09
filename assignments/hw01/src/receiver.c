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

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr,"Usage: %s <server_port>\n", argv[0]);
        exit(1);
    }

    const int server_port=atoi(argv[1]);

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len=sizeof(client_addr);

    sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
        error("Error opening socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(server_port);

    if(bind(sockfd,(struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        error("Error on binding");

    FILE *file=NULL;
    bool start_received=false;
    bool stop_received=false;

    myPacket_t packet;
    uint32_t file_size=0;

    while(! stop_received) {
        ssize_t recv_len=recvfrom(sockfd, &packet, sizeof(myPacket_t), 0, (struct sockaddr *) &client_addr,&client_len);
        if(recv_len < 0)
            error("Error receiving data");

        switch(packet.type){
            case NAME:
                file=fopen(packet.dataPacket.data, "w");
                fprintf(stderr,"INFO: file name - %s\n",packet.dataPacket.data);
                break;
            case SIZE:
                size_t i=0;
                for(;;){
                    if(packet.dataPacket.data[i]=='\0')
                        break;
                    file_size*=10;
                    file_size+=packet.dataPacket.data[i++] - '0';
                }
                fprintf(stderr,"INFO: file size - %d\n",file_size);
                break;
            case START:
                start_received=true;
                fprintf(stderr,"INFO: START received!\n");
                break;
            case DATA:
                if (!file || !start_received)
                    error("Error: data before START, exiting..\n");
                fwrite(packet.dataPacket.data, 1, recv_len - sizeof(short)*2, file); //- sizeof(short) * 2
                break;
            case END:
                stop_received=true;
                break;
            default:
                fprintf(stderr, "Error: instruction %d with data %s not implemented!\n", packet.type, packet.dataPacket.data);
        }
    }
    fclose(file);
    close(sockfd);
    return 0;
}

void error(const char *msg){
    perror(msg);
    exit(2);
}
