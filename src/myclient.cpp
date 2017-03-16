#include <sys/socket.h>  // for socket() and bind()
#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <unistd.h>      // for close()

#include <cstring>       // for memset()
#include <string>		 // for string
#include <iostream>      // for io

#include <utility.cpp>   // for dieWithError()
#include <RDTSegment.h>

#include <time.h> //for timeval

using namespace std;

struct ClientPacketModule {
  bool received_; //The version of c++ we are using forbids bool received_ = false; in the class definition, so make sure to set this whenever an object of this class type is made
  RDTSegment segment;
};

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

	// Prepare SYN to send
    struct RDTSegment synSeg;
    memset(&synSeg, 0, sizeof(struct RDTSegment));
    setSyn(&synSeg.header);
    toNetwork(&synSeg.header);

    //Declare an fd_set that will be used for entire "connection"
    //readfds is used for select() to monitor our socket
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    //Timeout information for use with select()
    struct timeval tv_synSeg;
    tv_synSeg.tv_sec = 0;
    tv_synSeg.tv_usec = (long) TIMEOUT_MS * 1000;

    //Send SYN, taking into account that the SYN may be lost
    while(true) {
      if (sendto(sockfd, &synSeg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	     dieWithError("ERROR, fail to send");
       cout << "Sending packet SYN\n";
      // Receive a response
      struct sockaddr_in fromAddr;
      unsigned int fromSize = sizeof(fromAddr);

      struct RDTSegment recvMsg;
      memset(&synSeg, 0, sizeof(struct RDTSegment));


      int synSeg_ret = select(sockfd + 1, &readfds, NULL, NULL, &tv_synSeg);
      if(synSeg_ret > 0) {
        //Received data
        if(recvfrom(sockfd, &recvMsg, sizeof(RDTSegment), 0, (struct sockaddr*) &fromAddr, &fromSize) < 0) {
          dieWithError("ERROR, fail to receive");
        }
        if(serverAddress.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
          dieWithError("ERROR, unknown server");
        }
        if(!isSyn(&recvMsg.header)) {
          //Received a message from the server that is not a SYN.
          cout << "Error, received a first message that is not a SYN";
        }
        //INCREMENT RECV_BASE
        cout << "Receiving packet 0" << endl;
        break;
      }
      else if(synSeg_ret == 0) {
        //Timout occured
      }
      else {
        dieWithError("ERROR, select() error");
      }

    } //while loop
    cout << "Client Received: " << endl;
    //print(&recvMsg);
    close(sockfd);
    return 0;
}
