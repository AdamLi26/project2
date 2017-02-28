#include <sys/socket.h>  // for socket() and bind()
#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <unistd.h>      // for close()

#include <cstring>       // for memset()
#include <string>        // for string
#include <iostream>      // for io

#include <utility.cpp>   // for dieWithError()
#include <RDTSegment.h>

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2)
        dieWithError("ERROR, expected only one argument to the program. <port number>");

    int serverPort = stoi(argv[1]);
    if (serverPort < 1 || serverPort > 65535)  // 0 is reserved
        dieWithError("ERROR, please provide the correct port number.");

    // create a UDP socket 
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) 
        dieWithError("ERROR, fail to creat a socket");

    // declare and initialize server address
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any IP address
    serverAddress.sin_port = htons(serverPort);

    // binds the local ip address and the port number to the newly created socket
    if (bind(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
        dieWithError("ERROR, fail to bind");

    struct sockaddr_in clientAddress;
	unsigned int clientAddressLenth;
	struct RDTSegment recvMsg;
    memset(&recvMsg, 0, sizeof(struct RDTSegment));

    while (true) {
    	// Set the size of the in-out parameter
    	clientAddressLenth = sizeof(clientAddress);

		// Block until receive message from a client 
		if (recvfrom(sockfd, &recvMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *) &clientAddress, &clientAddressLenth) < 0)
			dieWithError("ERROR, fail to receive!");
		// cout << "Handling client: " << inet_ntoa(clientAddress.sin_addr) << endl;
        toLocal(&recvMsg.header);
        cout << "Server Received: " << endl;
        print(&recvMsg);
        cout << 0;
        ack(&recvMsg);

		// Send received datagram back to the client  
		if (sendto(sockfd, &recvMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0) 
			dieWithError("ERROR, fail to send");
    }

    return 0; // never reached
}