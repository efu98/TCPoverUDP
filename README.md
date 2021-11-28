# TCPoverUDP

## Description
The goal of this projet is to design and implement servers adapted to three scenarios :
 - The first scenario uses a client of type client1.
 - The second scenario uses a client of type client2.
 - The third scenario uses multiple clients of type client1.

For all the scenarios, the metric used to measure the performances of the servers are their throughput. 

The average throughput obtained for the three scenarios are respectively : 2MB/s, 0.86MB/s, 2MB/s

### TCP features added :
 - Three way handshake
 - Sliding window
 - Fast retransmit

## Executing program

* How to run the program
* To run the server :
```
make
 ./serveur<number>-TheCoolPeople <port number>
```

* To run the clients :
```
 ./client<number> <ip address of the server> <server port number> <path to file>
```

## Authors

Elise Fu  
Mathis Langlois

