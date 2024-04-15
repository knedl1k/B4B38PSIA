## Task
Extend the application from the first task to detect data corruption during transmission. The receiving application must be able to detect file transmission errors at both the whole file level and at the individual packet level. Use a hash function (SHA, MD5, ...) to detect a transmission error at the file level, and a CRC function (e.g. generating a CRC-32 polynomial) at the packet level. You do not have to implement the hash and CRC calculation yourself, but you can use a ready-made library (try Google).

In addition, implement a stop-and-wait algorithm. The sender sends just one packet and waits for confirmation from the receiver that the data was received correctly (including CRC checking). Only after a positive acknowledgement is received is it allowed to send another packet. If an error (wrong CRC) is detected by the receiver, the receiver sends a negative acknowledgement to the sender and the sender resends the packet. The loss of the acknowledgement or the packet itself will be handled by a suitable timeout on the sender's side, after which the packet will be resent. The receiver must handle the situation where only the acknowledgement is lost and the same packet is therefore received multiple times.

Demonstrate your solution's ability to correctly respond to data transmission errors using NetDerper.

Bonus part

Discuss the advantages and especially the disadvantages of the Stop-and-Wait method when the end-to-end delay between nodes increases. Mathematically describe the effect of Stop-and-Wait on data throughput as a function of delay and demonstrate the (sufficient) correspondence of your description with reality using NetDerper.

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
