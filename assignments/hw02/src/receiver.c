// GNU General Public License v3.0
// @knedl1k & @
/*
 * file receiver
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <openssl/sha.h>

#include "packets.h"
#include "hash.h"



void error(const char *msg);

void managePacketStream(int sockfd, const struct sockaddr_in *client_addr);

void printSHAsum(unsigned char* md) {
    int i;
    for(i=0; i <SHA256_DIGEST_LENGTH; i++) {
        printf("%02x",md[i]);
    }
}

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
    FILE *fd=NULL;
    bool start_received=false;
    bool hash_check=false;

    myPacket_t packet;
    uint32_t file_size=0;
    char *file_name=NULL;
    uint32_t received_size=0;
    socklen_t client_len=sizeof(client_addr);

    while(! hash_check){
        ssize_t recv_len=recvfrom(sockfd, &packet, sizeof(myPacket_t), //TODO handle the return value?
                                  0,(struct sockaddr *) &client_addr,&client_len);
        if(recv_len < 0)
            error("Error receiving data");

        switch(packet.type){
            case NAME: ;
                size_t file_name_len=strlen((char*)packet.dataPacket.data);
                file_name=malloc(file_name_len); //TODO handle the return value
                //printf("strl:%ld, siof:%ld\n",strlen((char*)packet.dataPacket.data), sizeof(packet.dataPacket.data));
                memcpy(file_name,packet.dataPacket.data, file_name_len);
                //file_name[file_name_len]='\0';
                //printf("%ld\n",strlen(file_name));

                fd=fopen(file_name, "w"); //TODO handle the return value?
                fprintf(stderr, "INFO: fd name - %s.\n", file_name);
                break;
            case SIZE: ;
                size_t i=0;
                for(;;){
                    if(packet.dataPacket.data[i] == '\0')
                        break;
                    file_size *= 10;
                    file_size += packet.dataPacket.data[i++] - '0';
                }
                fprintf(stderr, "INFO: fd size - %d.\n", file_size);
                break;
            case START:
                start_received=true;
                fprintf(stderr, "INFO: START received!\n");
                break;
            case DATA:
                if(!fd || !start_received){
                    hash_check=true;
                    fprintf(stderr,"Error: data before START, exiting..\n");
                }
                if(received_size + sizeof(packet.dataPacket.data) < file_size){
                    fwrite(packet.dataPacket.data, 1, sizeof(packet.dataPacket.data), fd); //TODO handle the return value? recv_len - sizeof(short) * 2
                    received_size+=sizeof(packet.dataPacket.data);
                }else{
                    fwrite(packet.dataPacket.data, 1, file_size - received_size, fd);
                    received_size=file_size;
                }
                break;
            case END:
                fclose(fd);
                break;
            case SHA256SUM: ;
                unsigned char sender_hash[SHA256_DIGEST_LENGTH];
                memcpy(sender_hash,packet.dataPacket.data,SHA256_DIGEST_LENGTH);
                unsigned char receiver_hash[SHA256_DIGEST_LENGTH];
                //printf("strlen:%ld, sizeof:%ld\n",strlen(file_name),sizeof(file_name));
                calculateHash(file_name, file_size, receiver_hash);
                if(memcmp(sender_hash, receiver_hash, SHA256_DIGEST_LENGTH)){
                    //TODO send ERROR to sender!
                    printf("HASH ERROR!\n");
                }else{
                    //TODO send OK to sender
                    hash_check=true;
                    printf("HASH OK!\n");
                }


                break;


            default:
                fprintf(stderr, "Error: instruction %d with data %s not implemented!\n",
                        packet.type, packet.dataPacket.data);
        }
    }

    free(file_name);
    //fclose(fd);
}

void error(const char *msg){
    perror(msg);
    exit(100);
}
