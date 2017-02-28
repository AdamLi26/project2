#include <sys/socket.h>  // for socket() and bind()
#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <unistd.h>      // for close()

#include <cstring>       // for memset()
#include <string>		 // for string 
#include <iostream>      // for io

#include <utility.cpp>   // for dieWithError()
#include <RDTSegment.h>

using namespace std;

int main(int argc, char *argv[]) 
{
	if (argc != 4)
        dieWithError("ERROR, expected exactly four argument to the program. <server_hostname> <server_portnumber> <filename>");

    int serverPort = stoi(argv[2]);
    if (serverPort < 1 || serverPort > 65535)  // 0 is reserved
        dieWithError("ERROR, please provide the correct port number.");

	// Create a UDP socket 
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0)
		dieWithError( "ERROR, fail to creat a socket") ;
	
	// declare and initialize server address
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]); 
    serverAddress.sin_port = htons(serverPort);

	// Send the string to the server
    struct RDTSegment synSeg;
    memset(&synSeg, 0, sizeof(struct RDTSegment));
    setSyn(&synSeg.header);
    toNetwork(&synSeg.header);

	if (sendto(sockfd, &synSeg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
		dieWithError("ERROR, fail to send");
	
	// Receive a response 
	struct sockaddr_in fromAddr;
	unsigned int fromSize = sizeof(fromAddr);

	struct RDTSegment recvMsg;
	memset(&synSeg, 0, sizeof(struct RDTSegment));

	if (recvfrom(sockfd, &recvMsg, sizeof(RDTSegment), 0, (struct sockaddr *) &fromAddr, &fromSize) < 0)
		dieWithError("ERROR, fail to receive");

	if (serverAddress.sin_addr.s_addr != fromAddr.sin_addr.s_addr) 
		dieWithError("ERROR, unknown server");

	cout << "Client Received: " << endl;
	print(&recvMsg);
	close(sockfd); 
	return 0;
}