#include <sys/socket.h>  // for socket() and bind()
#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <unistd.h>      // for close()

#include <cstring>       // for memset()
#include <string>		 // for string
#include <iostream>      // for io

#include <utility.cpp>   // for dieWithError()
#include <RDTSegment.h>

#include <time.h> //for timeval
#include <deque>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> //For opening / writing files

#include <unordered_set>

using namespace std;

class ClientPacketModule {
public:
  bool received_; //The version of c++ we are using forbids bool received_ = false; in the class definition, so make sure to set this whenever an object of this class type is made
  struct RDTSegment segment;
  unsigned int amount_of_data;
};

const uint16_t CLIENT_INITIAL_SEQUENCE_NUM = 0;
unsigned int DATA_PER_PACKET = MAX_SEGMENT_SIZE - sizeof(RDTHeader);

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

    // For response
    struct sockaddr_in fromAddr;
    unsigned int fromSize = sizeof(fromAddr);

	// Prepare SYN to send
    struct RDTSegment synSeg;
    memset(&synSeg, 0, sizeof(struct RDTSegment));
    setSyn(&synSeg.header);
    synSeg.header.seqNum = CLIENT_INITIAL_SEQUENCE_NUM;
    //toNetwork(&synSeg.header);

    //Declare an fd_set that will be used for entire "connection"
    //readfds is used for select() to monitor our socket
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    //Timeout information for use with select()
    struct timeval tv_synSeg;
    tv_synSeg.tv_sec = 0;
    tv_synSeg.tv_usec = (long) TIMEOUT_MS * 1000;

    //SR Protocol information
    uint16_t recv_base = CLIENT_INITIAL_SEQUENCE_NUM;
    uint16_t recv_end = 0;
    deque<ClientPacketModule> recv_buffer;
    uint32_t file_size = 0;
    uint32_t file_size_received_so_far = 0;



    struct RDTSegment recvMsgSYN;
    memset(&recvMsgSYN, 0, sizeof(struct RDTSegment));
    /********************************************************
    I. SEND SYN AND RECEIVE SYNACK
    Take into acount that either one may be lost
    ********************************************************/
    while(true) {
      if (sendto(sockfd, &synSeg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	     dieWithError("ERROR, fail to send");
       cout << "Sending packet SYN\n";




      tv_synSeg.tv_usec = (long) TIMEOUT_MS * 1000;
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(sockfd, &readfds);
      int synSeg_ret = select(sockfd + 1, &readfds, NULL, NULL, &tv_synSeg);
      if(synSeg_ret > 0) {
        //Received data
        if(recvfrom(sockfd, &recvMsgSYN, sizeof(RDTSegment), 0, (struct sockaddr*) &fromAddr, &fromSize) < 0) {
          dieWithError("ERROR, fail to receive");
        }
        if(serverAddress.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
          dieWithError("ERROR, unknown server");
        }
        if(recvMsgSYN.header.seqNum != recv_base)
          continue;
        // toLocal(&recvMsgSYN.header);
        if(!isSyn(&recvMsgSYN.header)) {
          //Received a message from the server that is not a SYN. Figure out whether to terminate or what here
          //print(&recvMsgSYN);
          dieWithError("Error, received a first message that is not a SYN\n");
        }
        cout << "Receiving packet " << recv_base << endl;
        recv_base++;
        break;
      }
      else if(synSeg_ret == 0) {
        //Timout occured. Retry connection
        continue;
      }
      else {
        dieWithError("ERROR, select() error");
      }

    } //while loop

    /********************************************************
    II. REQUEST THE FILE AND RECEIVE THE FILE SIZE
    ********************************************************/
    while(true) {

      struct RDTSegment file_request_segment;
      memset(&file_request_segment, 0, sizeof(struct RDTSegment));
      file_request_segment.header.seqNum = recv_base;
      file_request_segment.header.ackNum = generateAck(&recvMsgSYN);
      setAck(&file_request_segment.header);

      cout << "Sending packet " << file_request_segment.header.ackNum << endl;
      strcpy(file_request_segment.data, argv[3]);
      //toNetwork(&file_request_segment.header);
      if (sendto(sockfd, &file_request_segment, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	     dieWithError("ERROR, fail to send");



      //toNetwork(&file_request_segment.header);

      struct RDTSegment recvMsg;
      memset(&recvMsg, 0, sizeof(struct RDTSegment));


      tv_synSeg.tv_usec = (long) TIMEOUT_MS * 1000;
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(sockfd, &readfds);
      int file_request_segment_ret = select(sockfd + 1, &readfds, NULL, NULL, &tv_synSeg);
      if(file_request_segment_ret > 0) {
        //Received data
        if(recvfrom(sockfd, &recvMsg, sizeof(struct RDTSegment), 0, (struct sockaddr*) &fromAddr, &fromSize) < 0) {
          dieWithError("ERROR, fail to receive");
        }
        if(serverAddress.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
          dieWithError("ERROR, unknown server");
        }
        if(recvMsg.header.seqNum != recv_base)
          continue;
        // toLocal(&recvMsg.header);
        cout << "Receiving packet " << recv_base << endl;
        memcpy(&file_size, recvMsg.data, 4);
        //file_size = ntohl(file_size);
        cout << "Preparing to receive file of size: " << file_size << endl;
        recv_base+=4;

        //ACK
        memset(&file_request_segment, 0, sizeof(struct RDTSegment));
        file_request_segment.header.seqNum = recv_base;
        file_request_segment.header.ackNum = generateAck(&recvMsg);
        setAck(&file_request_segment.header);

        cout << "Sending packet " << file_request_segment.header.ackNum << endl;
        if (sendto(sockfd, &file_request_segment, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  	     dieWithError("ERROR, fail to send");

        break;
      }
      else if(file_request_segment_ret == 0) {
        //Timout occured. Retry file request
        continue;
      }
      else {
        dieWithError("ERROR, select() error");
      }
    } //while(true)

     unordered_set<uint16_t> already_ACKED;

      //Open / create the file
      int fd = open("received.data", O_WRONLY | O_CREAT | O_TRUNC);
      if(fd<0)
        dieWithError("Error: Could not open the file. Please restart");

      uint32_t loops = file_size / (MAX_SEQ_NUM+1);
      uint32_t last_seqNum = recv_base + (file_size / SEGMENT_PAYLOAD_SIZE) * SEGMENT_PAYLOAD_SIZE;
      last_seqNum = (last_seqNum % (MAX_SEQ_NUM+1)); //-loops;

      //Initialize recv_buffer, which represents a window
      unsigned int window_size = WINDOW_SIZE / MAX_SEGMENT_SIZE;
      //I moved the creation of the module outside the for-loop to save on time
      //I believe push_back() uses copying so this is okay

      for(unsigned int i=0; i < window_size; i++) {
        ClientPacketModule module;
        module.received_ = false;
        memset(&module.segment, 0, sizeof(RDTSegment));
        module.segment.header.seqNum = recv_base + i*MAX_SEGMENT_SIZE;
        recv_buffer.push_back(module);
      }

      recv_end = recv_base + WINDOW_SIZE;

      /********************************************************
      III. TRANSFER OF DATA
      ********************************************************/
      while(true) {
        struct RDTSegment recvMsg;
        memset(&recvMsg, 0, sizeof(struct RDTSegment));

        struct RDTSegment ackMsg;
        memset(&ackMsg, 0, sizeof(struct RDTSegment));
        setAck(&ackMsg.header);

        if(recvfrom(sockfd, &recvMsg, sizeof(RDTSegment), 0, (struct sockaddr*) &fromAddr, &fromSize) < 0) {
          dieWithError("ERROR, fail to receive");
        }
        // toLocal(&recvMsg.header);

        /****************************
        Path 1: Message is a FIN
        ****************************/
        if(isFin(&recvMsg.header)) {
          close(fd);

          ackMsg.header.ackNum = recvMsg.header.seqNum;
          setAck(&recvMsg.header);
          cout << "Sending packet " << ackMsg.header.ackNum << endl;
          //toNetwork(&ackMsg.header);
          if (sendto(sockfd, &ackMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
              dieWithError("ERROR, fail to send");

          struct RDTSegment finMsg;
          memset(&finMsg, 0, sizeof(struct RDTSegment));
          setFin(&finMsg.header);
          uint16_t final_seqNum = ackMsg.header.ackNum + MAX_SEGMENT_SIZE;
          if(final_seqNum > MAX_SEQ_NUM)
            final_seqNum = MAX_SEQ_NUM - final_seqNum;
          cout << "Sending packet " << final_seqNum << " FIN\n";
          finMsg.header.seqNum = final_seqNum;
          //toNetwork(&finMsg.header);
          if (sendto(sockfd, &finMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
              dieWithError("ERROR, fail to send");

          cout << "last seqNum: " << last_seqNum << endl;
          //Wait for an ACK, retransmitting FIN on Timeout
          //If 30 seconds passes, just close the program
          for(int i=0; i < 10; i++) {
            //Reuse synSeg, which contains the timeout value
            memset(&recvMsg, 0, sizeof(RDTSegment));
            tv_synSeg.tv_usec = (long) TIMEOUT_MS * 1000;
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            int fin_seg_ret = select(sockfd + 1, &readfds, NULL, NULL, &tv_synSeg);
            if(fin_seg_ret > 0) {
              if(recvfrom(sockfd, &recvMsg, sizeof(RDTSegment), 0, (struct sockaddr*) &fromAddr, &fromSize) < 0) {
                dieWithError("ERROR, fail to receive");
            }
              // toLocal(&recvMsg.header);
              if(recvMsg.header.ackNum == final_seqNum) {
                cout << "Receiving packet " << final_seqNum << endl;
                close(sockfd);
                return 0;
            }
            else {
              struct RDTSegment extraMsg;
              memset(&extraMsg, 0, sizeof(struct RDTSegment));
              setAck(&extraMsg.header);
              extraMsg.header.ackNum = generateAck(&recvMsg);
              cout << "Sending packet " << extraMsg.header.seqNum << endl;
              if (sendto(sockfd, &extraMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
                  dieWithError("ERROR, fail to send");
            }
            }
          else {
              //toNetwork(&finMsg.header);
              if (sendto(sockfd, &ackMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
                  dieWithError("ERROR, fail to send");
              if (sendto(sockfd, &finMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
                dieWithError("ERROR, fail to send");
          }
          }
          close(sockfd);
          return 0;

        }
        /****************************
        Path 2: Message is a packet that is outside the current window
        Note: Must also check if the window is circling back around
        ****************************/
        else if(  (already_ACKED.find(recvMsg.header.seqNum) != already_ACKED.end()) ||
                      (   ((int)recv_end - (int)recv_base > 0) && (  (recvMsg.header.seqNum < recv_base) || ( recvMsg.header.seqNum > recv_end)  ) ) ||
                      (    ((int)recv_end - (int)recv_base < 0) && (  (recvMsg.header.seqNum < recv_base) && (recvMsg.header.seqNum > recv_end) ) ) ) {
          //recvMsg.header.seqNum < recv_base && ( (recv_end < recv_base &&  recvMsg.header.seqNum > recv_end)
            //          || (recv_end > recv_base && recvMsg.header.seqNum < recv_end) ) ) {
          ackMsg.header.ackNum = recvMsg.header.seqNum;
          setAck(&recvMsg.header);
          cout << "Sending packet " << ackMsg.header.ackNum << " Retransmission" << endl;
          //toNetwork(&ackMsg.header);
          if (sendto(sockfd, &ackMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
              dieWithError("ERROR, fail to send");
          continue;
        }
        /****************************
        Path 3: Receive packet within our window
        ****************************/
        else {
          //Determine which element in our recv_buffer corresponds to the packet received
          uint16_t index = recvMsg.header.seqNum >= recv_base ? (recvMsg.header.seqNum - recv_base) / 1024 :
                                                                                                                (MAX_SEQ_NUM - recv_base + recvMsg.header.seqNum) / 1024;
          recv_buffer[index].received_ = true;
          ackMsg.header.ackNum = recvMsg.header.seqNum;
          setAck(&recvMsg.header);

          //Determine the amount of file data read and move that much data into a buffer to be written later
          unsigned int data_left = file_size - file_size_received_so_far;
          bool write_less_data = false;
          if(recvMsg.header.seqNum == last_seqNum) {
            if(loops == 0) write_less_data = true;
          }
          unsigned int amount_of_data = write_less_data ? data_left : DATA_PER_PACKET;
          file_size_received_so_far += amount_of_data;
          memcpy(recv_buffer[index].segment.data, recvMsg.data, amount_of_data); //bug
          recv_buffer[index].amount_of_data = amount_of_data;
          recv_buffer[index].segment.header.seqNum = recvMsg.header.seqNum;
          already_ACKED.insert(recvMsg.header.seqNum);

          // //If data received is less than the size of the data buffer, set the null byte
          // if(amount_of_data == data_left) {
          //   recv_buffer[index].segment.data[amount_of_data] = '\0';
          // }

          //If the sequence number just received is the left bound of our window, we can move the window right
          if(recvMsg.header.seqNum == recv_base) {}
            int buffer_size = recv_buffer.size();
            for(int i=0; i<buffer_size; i++) {
              //Move window right while the leftmost packet has already been acked
              if(recv_buffer.front().received_) {
                //Write to file
                //int amount_to_write = recv_buffer.front().amoun;
                write(fd, recv_buffer.front().segment.data, recv_buffer.front().amount_of_data);
                already_ACKED.erase(recv_buffer.front().segment.header.seqNum);


                recv_base += SEGMENT_PAYLOAD_SIZE;
                if(recv_base > MAX_SEQ_NUM) {
                  recv_base -=  MAX_SEQ_NUM;
                  recv_base--;
                  loops--;
                }
                recv_end += SEGMENT_PAYLOAD_SIZE;
                if(recv_end > MAX_SEQ_NUM) {
                  recv_end -= MAX_SEQ_NUM;
                  recv_end--;
                }
                recv_buffer.pop_front();
                //Add a new ClientPacketModule to track for new packets
                ClientPacketModule next_packet;
                next_packet.received_ = false;
                memset(&next_packet.segment, 0, sizeof(RDTSegment));
                recv_buffer.push_back(next_packet);
              }
              else break;
            }

          }
          //Finally, ACK the packet we just received
          cout << "Sending packet " << ackMsg.header.ackNum << endl;
          //toNetwork(&ackMsg.header);
          if (sendto(sockfd, &ackMsg, sizeof(struct RDTSegment), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
              dieWithError("ERROR, fail to send");
        }
      //} //while(true)
    close(sockfd);
    dieWithError("Got to the end of main function. Not supposed to happen!");
    return 0;
}
