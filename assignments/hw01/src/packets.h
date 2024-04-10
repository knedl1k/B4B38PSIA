#ifndef PACKETS_H
#define PACKETS_H

#define PACKET_MAX_LEN 1024
#define SENDTO_ERROR -1

enum{
    NAME=0,
    SIZE,
    START,
    DATA,
    END
};

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

#define SEND(sockfd,send_buffer,len,server_addr) \
                (sendto(sockfd, send_buffer, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)))

#endif