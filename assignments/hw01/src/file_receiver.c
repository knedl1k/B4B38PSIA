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

    while (! stop_received){
        char recv_buffer[PACKET_MAX_LEN];
        ssize_t recv_len=recvfrom(sockfd, recv_buffer, PACKET_MAX_LEN, 0, (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0)
            error("Error receiving data");
        
        recv_buffer[recv_len]='\0';

        if (strncmp(recv_buffer, "NAME=", 5) == 0){
            sscanf(recv_buffer, "NAME=%s", filename);
            file=fopen(filename, "wb");
            if (file == NULL)
                error("Error creating file");
            
        }else if (strncmp(recv_buffer, "SIZE=", 5) == 0){
            // We can use SIZE if necessary
        }else if (strncmp(recv_buffer, "START", 5) == 0){
            // We can handle START if necessary
        }else if (strncmp(recv_buffer, "STOP", 4) == 0){
            stop_received=1;
        }else{
            uint32_t position;
            memcpy(&position, recv_buffer, sizeof(uint32_t));
            position=ntohl(position);
            if (position != expected_position){
                fprintf(stderr, "Received out-of-order data, ignoring\n");
                continue;
            }
            fwrite(recv_buffer + 4, 1, recv_len - 4, file);
            expected_position += recv_len - 4;
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