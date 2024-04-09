// GNU General Public License v3.0
// @knedl1k
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

#define PACKET_MAX_LEN 1024
#define SEND(sockfd,send_buffer,len,server_addr) (sendto(sockfd, send_buffer, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)))

typedef struct{
    unsigned char data[PACKET_MAX_LEN-4];
}myDataPacket_t;

typedef struct{
    short type;
    short crc;
    union{
        myDataPacket_t dataPacket;
    };
}myPacket_t;


void error(const char *msg);


int main(int argc, char *argv[]){
    if (argc < 5){
        fprintf(stderr,"Usage: %s <server_ip> <server_port> <filename> <client_port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip=argv[1];
    const int server_port=atoi(argv[2]);
    char *filename=argv[3];
    const int client_port=atoi(argv[4]);

    FILE *file=fopen(filename, "rb");
    if (file == NULL)
        error("Error opening file");

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len=sizeof(client_addr);

    sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("Error opening socket");

    memset(&client_addr, '\0', sizeof(client_addr));
    client_addr.sin_family=AF_INET;
    client_addr.sin_addr.s_addr=INADDR_ANY;
    client_addr.sin_port=htons(client_port);

    if (bind(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0)
        error("Error on binding");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(server_port);
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0)
        error("Invalid server IP address");
    
    myPacket_t packet;
    packet.type=3;
    packet.crc=0;
    memset(packet.dataPacket.data, 0, sizeof(packet.dataPacket.data));

    //TODO THIS SECTION MUST BE CHANGED TO ME MORE UNIVERSAL
    myPacket_t packet2={.type=0, .crc=0, .dataPacket.data=*filename};
    //printf("%s\n",packet2.dataPacket.data);
    sendto(sockfd, (char*)&packet2, sizeof(myPacket_t), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    //sprintf(send_buffer, "SIZE=%ld\n", ftell(file));
    packet2.type=1;
    packet2.crc=0;

    fseek(file, 0L, SEEK_END);
    printf(">>%d\n",ftell(file));
    printf(">%d\n",htonl(ftell(file)));
    sprintf(packet2.dataPacket.data, "%ld",htonl(ftell(file)));
    fseek(file, 0L, SEEK_SET);
    //printf("%s\n",packet2.dataPacket.data);
    sendto(sockfd, (char*)&packet2, sizeof(myPacket_t), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    //sprintf(send_buffer, "START\n");
    packet2.type=2;
    packet2.crc=0;
    memset(packet2.dataPacket.data, 0, sizeof(packet2.dataPacket.data));
    sendto(sockfd, (char*)&packet2, sizeof(myPacket_t), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    //TODO THIS SECTION MUST BE CHANGED TO ME MORE UNIVERSAL


    while(! feof(file)){
         fread(packet.dataPacket.data, PACKET_MAX_LEN-4, 1, file);
         sendto(sockfd, (char*) &packet, sizeof(myPacket_t), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
         //printf("%s\n",packet.dataPacket.data);
         //sleep(2);
    }


    //size_t bytes_read;

    /*
    sprintf(send_buffer, "STOP\n");
    SEND(sockfd,send_buffer,strlen(send_buffer),server_addr);
    */

    packet2.type=9;
    packet2.crc=0;
    memset(packet2.dataPacket.data, 0, sizeof(packet2.dataPacket.data));
    sendto(sockfd, (char*)&packet2, sizeof(myPacket_t), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    fclose(file);
    close(sockfd);
    return 0;
}

void error(const char *msg){
    perror(msg);
    exit(1);
}
