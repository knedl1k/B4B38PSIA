// GNU General Public License v3.0
// @knedl1k
/*
 * everything about packets
 *
 */
#ifndef PACKETS_H
#define PACKETS_H

#define PACKET_MAX_LEN 1024
#define PACKET_OFFSET (sizeof(short)+sizeof(uint32_t))
#define SENDTO_ERROR (-1)

enum{
    NAME=0,
    SIZE,
    START,
    DATA,
    SHA256SUM,
    OK,
    ERROR,
    END
};

typedef struct{
    unsigned char data[PACKET_MAX_LEN-PACKET_OFFSET];
}myDataPacket_t;

typedef struct{
    short type;
    union{
        myDataPacket_t dataPacket;
    };
    uint32_t crc;
}__attribute__((packed)) myPacket_t;

#define SEND(sockfd,send_buffer,len,server_addr) \
                (sendto(sockfd, send_buffer, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)))

#endif