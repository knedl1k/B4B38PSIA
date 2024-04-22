## Task
Replace the Stop-and-Wait method from hw2 with the Selective Repeat method. The sender will sequentially send multiple packets (up to N) and receive acknowledgement from the receiver. The number of packets sent and not yet acknowledged must not exceed N (the size of the so-called transmit window). In case of an error in one packet, this (and only this) packet will be resent. Using NetDerper, demonstrate the ability of your solution to respond to any errors or packet losses.

Bonus part

Discuss the effect of the size of the Selective Repeat method's transmit window in relation to the increasing end-to-end delay between nodes. Deduce the minimum transmit window size for a given delay and use NetDerper to show that further increases in the size of the transmit window no longer really affect the achieved throughput.

The submission of both parts is conditional on the demonstration of the communication flow of the created applications in Wireshark.


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
