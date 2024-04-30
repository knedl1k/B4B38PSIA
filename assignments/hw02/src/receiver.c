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
#include <errno.h>


void error(const char *msg);

void managePacketStream(int sockfd);

bool checkCRC(myPacket_t packet);


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(1);
    }

    const int local_port = atoi(argv[1]);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    /*struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        error("setsockopt failed");*/

    //TODO move to separate function
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0)
        error("Error on binding");
    //TODO

    managePacketStream(sockfd);

    close(sockfd);
    return 0;
}

bool checkCRC(myPacket_t packet) {
    uint32_t rec_CRC = crc_32((unsigned char *) &packet, sizeof(packet) - sizeof(packet.crc));
    //printf("rec%u : sed%u\n",rec_CRC,packet.crc);
    return rec_CRC == packet.crc;
}

void managePacketStream(const int sockfd) {
    FILE *fd = NULL;
    bool start_received = false;
    bool hash_check = false;

    myPacket_t packet;

    myPacket_t packet2;
    packet2.type = OK;
    memset(packet2.dataPacket.data, 0, sizeof(packet2.dataPacket.data));
    packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));

    uint32_t file_size = 0;
    char *file_name = NULL;
    uint32_t received_size = 0;
    bool lastNum = 0;
    int lastType = -1;

    struct sockaddr_in from;
    unsigned int fromlen = sizeof(from);

    while (!hash_check) {
        ssize_t recv_len = recvfrom(sockfd, &packet, sizeof(myPacket_t), //TODO handle the return value?
                                    0, (struct sockaddr *) &from, &fromlen);
        if (recv_len < 0)
            error("Error receiving data");

        if (!checkCRC(packet)) { //CRC does not match
            packet2.type = ERROR;
            packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
            SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);
            continue;
        }

        if (packet.num == lastNum && packet.type == lastType) {
            packet2.type = OK;
            packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
            SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);
            continue;
        }

        switch (packet.type) {
            case NAME:;
                packet2.type = OK;
                packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
                SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);

                size_t file_name_len = strlen((char *) packet.dataPacket.data);
                file_name = malloc(file_name_len); //TODO handle the return value
                memcpy(file_name, packet.dataPacket.data, file_name_len);

                fd = fopen(file_name, "w"); //TODO handle the return value?
                fprintf(stderr, "INFO: file name - %s.\n", file_name);
                break;
            case SIZE:;
                packet2.type = OK;
                packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
                SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);

                size_t i = 0;
                for (;;) {
                    if (packet.dataPacket.data[i] == '\0')
                        break;
                    file_size *= 10;
                    file_size += packet.dataPacket.data[i++] - '0';
                }
                fprintf(stderr, "INFO: file size - %d B.\n", file_size);
                break;
            case START:
                packet2.type = OK;
                packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
                SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);

                start_received = true;
                fprintf(stderr, "INFO: START received!\n");
                break;
            case DATA:
                //fprintf(stderr, "INFO: data packet received.\n");
                if (!fd || !start_received) {
                    hash_check = true;
                    fprintf(stderr, "Error: data before START, exiting..\n");
                }

                packet2.type = OK;
                packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
                SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);

                if (received_size + sizeof(packet.dataPacket.data) < file_size) {
                    fwrite(packet.dataPacket.data, 1, sizeof(packet.dataPacket.data),
                           fd); //TODO handle the return value? recv_len - sizeof(short) * 2
                    received_size += sizeof(packet.dataPacket.data);
                } else {
                    fwrite(packet.dataPacket.data, 1, file_size - received_size, fd);
                    received_size = file_size;
                }
                break;
            case END:
                packet2.type = OK;
                packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
                SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);

                fclose(fd);
                break;
            case SHA256SUM:;
                //printf("%d - %d\n",packet.crc, crc_32((unsigned char *)&packet, sizeof(packet)-sizeof(packet.crc) ));
                packet2.type = OK;
                packet2.crc = crc_32((unsigned char *) &packet2, sizeof(packet2) - sizeof(packet2.crc));
                SEND(sockfd, (char *) &packet2, sizeof(myPacket_t), from);

                unsigned char sender_hash[SHA256_DIGEST_LENGTH];
                memcpy(sender_hash, packet.dataPacket.data, SHA256_DIGEST_LENGTH);
                unsigned char receiver_hash[SHA256_DIGEST_LENGTH];
                calculateHash(file_name, file_size, receiver_hash);

                if (memcmp(sender_hash, receiver_hash, SHA256_DIGEST_LENGTH))
                    fprintf(stderr, "INFO: SHA256sum mismatch!\n");
                else
                    fprintf(stderr, "INFO: SHA256sum OK!\n");

                hash_check = true;

                break;

            default:
                fprintf(stderr, "Error: instruction %d with data %s not implemented!\n",
                        packet.type, packet.dataPacket.data);
        }
    }

    free(file_name);
    //fclose(fd);
}

void error(const char *msg) {
    perror(msg);
    exit(100);
}
