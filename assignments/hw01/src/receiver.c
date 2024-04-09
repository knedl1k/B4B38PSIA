// GNU General Public License v3.0
// @knedl1k
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

#define PACKET_MAX_LEN 1024

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

//setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(int))

void error(const char *msg);

int main(int argc, char *argv[]){
    if (argc < 2){
        fprintf(stderr,"Usage: %s <server_port>\n", argv[0]);
        exit(1);
    }

    const int server_port=atoi(argv[1]);

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len=sizeof(client_addr);

    sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("Error opening socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(server_port);

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        error("Error on binding");

    FILE *file=NULL;
    char filename[256];
    uint32_t expected_position=0;
    int stop_received=0;



    myPacket_t packet;

    int start_received = 0;
    uint32_t file_size = 0;

while (! stop_received){
    ssize_t recv_len=recvfrom(sockfd, &packet, sizeof(myPacket_t), 0, (struct sockaddr *)&client_addr, &client_len);
    if (recv_len < 0)
        error("Error receiving data");
    if(packet.type==0){
        file=fopen(packet.dataPacket.data,"w");
    }
    if (packet.type == 1){ // Assuming type 1 is for file size
        if (!start_received)
            error("Received file size before start signal");

        if (recv_len < sizeof(uint32_t))
            error("Received packet is too small for file size");

        memcpy(&file_size, packet.dataPacket.data, sizeof(uint32_t));
        file_size = ntohl(file_size); // Convert from network byte order to host byte order
    } else if (packet.type == 2){ // Assuming type 2 is for START
        start_received = 1;
    } else if (packet.type == 3) { // Assuming type 3 is for data
        if (!start_received)
            error("Received data before start signal");

        fwrite(packet.dataPacket.data, 1, recv_len - sizeof(short) * 2, file);
    } else if (packet.type == 9){
        stop_received = 1;
        break;
    }
    else {
        error("Received unexpected instruction!\n");
    }
}

    fclose(file);
    close(sockfd);
    return 0;
}

void error(const char *msg){
    perror(msg);
    exit(1);
}
