#include <sys/socket.h>  // for socket() and bind()
#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <unistd.h>      // for close()
#include <fcntl.h> 
#include <sys/file.h>

#include <cstring>       // for memset()
#include <cmath>
#include <string>        // for string
#include <iostream>      // for io
#include <fstream>       // for file I/O
#include <streambuf>     // for converting file stream to string
#include <deque>
#include <queue>
#include <chrono>

#include <utility.cpp>   // for dieWithError()
#include <RDTSegment.h>
#include <Packet.h>

using namespace std;
using namespace std::chrono;

enum state {CLOSED, LISTEN, SYN_RCVD, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, TIME_WAIT};
const seconds TIME_WAIT_DURATION = seconds(30);
const milliseconds RETRANSMISSION_TIME_OUT = milliseconds(500);
const size_t send_window_size = 5;

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

    // change the socket to nonblocking
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) 
        dieWithError("ERROR, unable to set to nonblocking");

    struct sockaddr_in clientAddress;
    unsigned int clientAddressLenth;
    struct RDTSegment recvSeg;
    memset(&recvSeg, 0, sizeof(struct RDTSegment));
    
    // initialize the state and buffers
    state server_state;

    deque<Packet> send_buffer;
    queue<Packet> packet_queue;
    unsigned int server_isn;
    monotonic_clock::time_point time_wait_start_time;
    uint32_t clientIP = 0; // 0 means not assgined

    server_state = LISTEN;

    while (true) {
        // Set the size of the in-out parameter
        clientAddressLenth = sizeof(clientAddress);

        // if did not receive message from a client, returns -1
        if (recvfrom(sockfd, &recvSeg, sizeof(struct RDTSegment), 0, (struct sockaddr *) &clientAddress, &clientAddressLenth) > 0) {
            // dieWithError("ERROR, fail to receive!");
            cout << "Receiving packet " << recvSeg.header.ackNum << endl;

            if (clientIP == 0) {
                clientIP= clientAddress.sin_addr.s_addr;
                cout << "Handling client: " << inet_ntoa(clientAddress.sin_addr) << endl;
            } else {
                if (clientIP != clientAddress.sin_addr.s_addr)
                    cerr << "Detect Second Client!" << endl;
            }
            
            toLocal(&recvSeg.header);
            // cout << "Server Received: " << endl;
            // print(&recvSeg);

            if (server_state == LISTEN && isSyn(&recvSeg.header)) {
                server_state = SYN_RCVD;
                server_isn = 0; // remember to randomize

                struct RDTSegment SYNACKSeg;
                memset(&SYNACKSeg, 0, sizeof(struct RDTSegment));
                SYNACKSeg.header.ackNum = generateAck(&recvSeg);
                setAck(&SYNACKSeg.header);
                setSyn(&SYNACKSeg.header);
                //toNetwork(&SYNACKSeg.header);
                Packet p(SYNACKSeg);
                packet_queue.emplace(p);
            } else if (server_state == SYN_RCVD && isAck(&recvSeg.header) && recvSeg.header.ackNum == server_isn + MAX_SEGMENT_SIZE) {
                /* only one SYNACK packet should be in send_buffer at this moment */
                send_buffer.pop_front();
                server_state = ESTABLISHED;

                // open file 
                string fileName(recvSeg.data);
                cout << "Openning File: " + fileName << endl;
                ifstream requestedFile(fileName); // open input file
                string requestedFileString = string((istreambuf_iterator<char>(requestedFile)), istreambuf_iterator<char>()); // max_size: 1073741820
                const char *requestedFile_c = requestedFileString.c_str();

                // file length
                uint32_t file_length_in_bytes = htonl(requestedFileString.size());
                int seqNum = server_isn + MAX_SEGMENT_SIZE;
                struct RDTSegment fileLengthSeg;
                memset(&fileLengthSeg, 0, sizeof(struct RDTSegment));
                fileLengthSeg.header.seqNum = seqNum;
                //toNetwork(&fileLengthSeg.header);
                memcpy(&fileLengthSeg.data, &file_length_in_bytes, sizeof(uint32_t));
                Packet p(fileLengthSeg);
                packet_queue.emplace(p);
                seqNum += MAX_SEGMENT_SIZE;

                // packetize the file and push it into packet_queue
                int total_num_packet = ceil(file_length_in_bytes/SEGMENT_PAYLOAD_SIZE);
                for (int i = 0; i < total_num_packet; ++i) {
                    struct RDTSegment fileDataSeg;
                    memset(&fileDataSeg, 0, sizeof(struct RDTSegment));
                    fileDataSeg.header.seqNum = seqNum;
                    //toNetwork(&fileDataSeg.header);
                    memcpy(&fileDataSeg.data, &requestedFile_c + i * SEGMENT_PAYLOAD_SIZE, SEGMENT_PAYLOAD_SIZE);
                    Packet p(fileDataSeg);
                    packet_queue.emplace(p);
                    seqNum += MAX_SEGMENT_SIZE;
                }

            } else if (server_state == ESTABLISHED && isAck(&recvSeg.header)) {
                for (auto& p : send_buffer) {
                    struct RDTSegment s = p.segment();
                    if (!p.isReceived() && recvSeg.header.ackNum == generateAck(&s)) {
                        p.markReceived();
                        while (send_buffer.front().isReceived()) {
                            send_buffer.pop_front();
                        }
                    }
                }
            } else if (server_state == FIN_WAIT_1 && isAck(&recvSeg.header)) {
                /* only one FIN packet should be in send_buffer at this moment */
                send_buffer.pop_front();
                server_state = FIN_WAIT_2;
            } else if (server_state == FIN_WAIT_2 && isFin(&recvSeg.header)) {
                server_state = TIME_WAIT;
                struct RDTSegment FINACKSeg;
                memset(&FINACKSeg, 0, sizeof(struct RDTSegment));
                FINACKSeg.header.ackNum = generateAck(&recvSeg);
                setAck(&FINACKSeg.header);
                //toNetwork(&FINACKSeg.header);
                Packet p(FINACKSeg);
                packet_queue.emplace(p);
            } else {
                // do nothing     
            }
        }

        auto nextPacketNumIter = send_buffer.end();
        while (send_buffer.size() < send_window_size && !packet_queue.empty()) {
            send_buffer.emplace_back(packet_queue.front());
            packet_queue.pop();
        }

        if (server_state == ESTABLISHED && send_buffer.empty()) {
            server_state = FIN_WAIT_1;

            struct RDTSegment FINSeg;
            memset(&FINSeg, 0, sizeof(struct RDTSegment));
            setFin(&FINSeg.header);
            //toNetwork(&FINSeg.header);
            Packet p(FINSeg);
            send_buffer.emplace_back(p); 
        }


        for (auto it = send_buffer.begin(); it < nextPacketNumIter; ++it) { 
            if (!it->isReceived() && duration_cast<milliseconds>(monotonic_clock::now() - it->sentTime()) > RETRANSMISSION_TIME_OUT) {
                it->updateSentTime();
                struct RDTSegment s = it->segment();
                if (sendto(sockfd, &s, sizeof(struct RDTSegment), 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0) 
                    dieWithError("ERROR, fail to send");
                cout << "Sending packet " << s.header.seqNum << " " << send_window_size;
                if (isSyn(&s.header)) 
                    cout << " SYN" << endl;
                else if (isFin(&s.header))
                    cout << " FIN" << endl;
                else
                    cout << endl;
            }
        }

        for (auto it = nextPacketNumIter; it < send_buffer.end(); ++it) {
            if (server_state == TIME_WAIT) 
                time_wait_start_time = monotonic_clock::now();
            it->updateSentTime();
            struct RDTSegment s = it->segment();
            if (sendto(sockfd, &s, sizeof(struct RDTSegment), 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0) 
                dieWithError("ERROR, fail to send");
            cout << "Sending packet " << s.header.seqNum << " " << send_window_size;
            if (isSyn(&s.header)) 
                cout << " SYN" << endl;
            else if (isFin(&s.header))
                cout << " FIN" << endl;
            else
                cout << endl;
        }

        if (server_state == TIME_WAIT && (duration_cast<seconds>(monotonic_clock::now() - time_wait_start_time) > TIME_WAIT_DURATION)) {
            server_state = CLOSED;
            break;
        }
    }

    return 0; // never reached
}