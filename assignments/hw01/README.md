## Task
The assignment is to create a console application that reads a file from one student's disk and sends it via UDP to the specified IP address and port of the other student's application in the group. The other student's application must correctly receive the file and save it to disk. 

The file transfer must meet the following parameters:
- UDP must be used (TCP is NOT allowed).
- The application must be able to transfer any file, not just text files. The submission task will be tested on sending any image with unlimited size.
- The transfer must consist of data packets with a maximum length of 1024 bytes
- The receiving application must safely recognize that the file transfer has completed and save the file to the disk of the receiving PC.

**Recommended sequence of data sent by the client:**
```
NAME=file.txt
SIZE=1152
START
DATA{4B position in file, binary (uint32_t)}{data 1B to PACKET_MAX_LEN - 4}
DATA{4B positions in file, binary (uint32_t)}{data 1B to PACKET_MAX_LEN - 4}
DATA{4B position in file, binary (uint32_t)}{data 1B to PACKET_MAX_LEN - 4}
...
STOP
```

## Usage
1. Compile both programs, e.g. using Makefile 
```
make
```
2. Start the receiver app, for e.g.:
```
./rec.bin 12345
```
3. Send the file via send app
\
\
 local server:
```
./send.bin 127.0.0.1 12345 file.txt 54321
```
> [!IMPORTANT]  
> Do not send the file to the same folder it is in! Data corruption may occur.
