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

#define PACKET_MAX_LEN 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,"Usage: %s <server_ip> <server_port> <filename> <client_port>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char *filename = argv[3];
    int client_port = atoi(argv[4]);

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        error("Error opening file");
    }

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(client_port);

    if (bind(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0) {
        error("Error on binding");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0) {
        error("Invalid server IP address");
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char send_buffer[PACKET_MAX_LEN];
    size_t bytes_read;

    sprintf(send_buffer, "NAME=%s\nSIZE=%ld\nSTART\n", filename, file_size);
    sendto(sockfd, send_buffer, strlen(send_buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    while ((bytes_read = fread(send_buffer + 12, 1, PACKET_MAX_LEN - 12, file)) > 0) {
        uint32_t position = htonl(ftell(file));
        memcpy(send_buffer, &position, sizeof(uint32_t));
        sendto(sockfd, send_buffer, bytes_read + 12, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    sprintf(send_buffer, "STOP\n");
    sendto(sockfd, send_buffer, strlen(send_buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    fclose(file);
    close(sockfd);
    return 0;
}

