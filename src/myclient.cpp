#include <sys/socket.h>  // for socket() and bind()
#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <unistd.h>      // for close()

#include <cstring>       // for memset()
#include <string>		 // for string 
#include <iostream>      // for io

#include <utility.cpp>   // for dieWithError()

using namespace std;

const size_t MAX_SEGMENT_SIZE = 1024;

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
	string sendMsg = "sent by client";
	if (sendto(sockfd, sendMsg.c_str(), sendMsg.size(), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
		dieWithError("ERROR, fail to send");
	
	// Recv a response 
	struct sockaddr_in fromAddr;
	unsigned int fromSize = sizeof(fromAddr);
	char recvMsg[MAX_SEGMENT_SIZE];
	memset(&recvMsg, 0, MAX_SEGMENT_SIZE);
	ssize_t n;
	if (( n = recvfrom(sockfd, recvMsg, MAX_SEGMENT_SIZE, 0, (struct sockaddr *) &fromAddr, &fromSize)) < 0) {
		cout << n << endl;
		// dieWithError("ERROR, fail to receive");
	}

	if (serverAddress.sin_addr.s_addr != fromAddr.sin_addr.s_addr) 
		dieWithError("ERROR, unknown server");

	close(sockfd); 

	cout << string(recvMsg) << endl;
	return 0;
}