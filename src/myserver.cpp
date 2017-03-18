#include <sys/socket.h>  // for socket() and bind()
#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <unistd.h>      // for close()
#include <fcntl.h>       // for changing the socket to nonblocking
#include <sys/file.h>

#include <cassert>       // for debug
#include <cstring>       // for memset()
#include <cmath>         // for ceil()
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
const milliseconds RETRANSMISSION_TIME_OUT = milliseconds(500);
const milliseconds TIME_WAIT_DURATION = 2 * RETRANSMISSION_TIME_OUT;
const size_t send_window_size = 5;

void sendToWithPrint(int sockfd, struct RDTSegment *s, size_t len, const struct sockaddr *dest_addr, socklen_t addrlen, bool printRetransmission) {
    if (sendto(sockfd, s, len, 0, dest_addr, addrlen) < 0)
        dieWithError("ERROR, fail to send");

    cout << "Sending packet " << s->header.seqNum << " " << send_window_size * MAX_SEGMENT_SIZE;
    if (printRetransmission)
        cout << " Retransmission";

    if (isSyn(&s->header))
        cout << " SYN" << endl;
    else if (isFin(&s->header))
        cout << " FIN" << endl;
    else
        cout << endl;
}

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

    /* change the socket to nonblocking */
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
        dieWithError("ERROR, unable to set to nonblocking");

    struct sockaddr_in clientAddress;
    unsigned int clientAddressLenth; /* the in-out parameter for recvfrom */
    struct RDTSegment recvSeg;
    memset(&recvSeg, 0, sizeof(struct RDTSegment));

    state server_state; /* server state */
    deque<Packet> send_buffer; /* all packets will be sent, always has <= send_window_size packet */
    queue<Packet> packet_queue; /* all packets need to be sent */
    unsigned int server_sn; /* server sequence number, needs to update every usage */
    monotonic_clock::time_point time_wait_start_time; /* for time wait mechanism */

    server_state = LISTEN;

    while (true) {
        clientAddressLenth = sizeof(clientAddress);

        /* Nonblocking: if did not receive message from a client, returns -1 */
        if (recvfrom(sockfd, &recvSeg, sizeof(struct RDTSegment), 0, (struct sockaddr *) &clientAddress, &clientAddressLenth) > 0) {
            /* output ACK number of receiving packet or SYN, FIN */
            cout << "Receiving packet ";
            if (isSyn(&recvSeg.header))
                cout << "SYN" << endl;
            else if (isFin(&recvSeg.header)) 
                cout << "FIN" << endl;
            else
                cout << recvSeg.header.ackNum << endl;

            if (server_state == LISTEN && isSyn(&recvSeg.header)) {
                server_state = SYN_RCVD;
                server_sn = 0; // remember to randomize

                struct RDTSegment SYNACKSeg;
                memset(&SYNACKSeg, 0, sizeof(struct RDTSegment));
                SYNACKSeg.header.ackNum = generateAck(&recvSeg);
                SYNACKSeg.header.seqNum = server_sn;
                server_sn = (server_sn + 1) % (MAX_SEQ_NUM + 1);
                setAck(&SYNACKSeg.header);
                setSyn(&SYNACKSeg.header);
                packet_queue.emplace(Packet(SYNACKSeg));
            } else if (server_state == SYN_RCVD && isAck(&recvSeg.header)) {
                assert(send_buffer.size() == 1); /* only one SYNACK packet should be in send_buffer at this moment */
                assert(recvSeg.header.ackNum == send_buffer.front().segment().header.seqNum); /* the ack received should ack the SYNACK */
                send_buffer.pop_front();
                server_state = ESTABLISHED;

                /* convert the file content to cstring ready to packetize */
                string fileName(recvSeg.data);
                cout << "Openning File: " + fileName << endl;
                ifstream requestedFile(fileName); // open input file
                string requestedFileString = string((istreambuf_iterator<char>(requestedFile)), istreambuf_iterator<char>()); // max_size: 1073741820
                const char *requestedFile_c = requestedFileString.c_str();
                
                /* packetize the file length and push it to the packet_queue */
                uint32_t file_length_in_bytes = requestedFileString.size();
                struct RDTSegment fileLengthSeg;
                memset(&fileLengthSeg, 0, sizeof(struct RDTSegment));
                fileLengthSeg.header.seqNum = server_sn;
                server_sn = (server_sn + sizeof(uint32_t)) % (MAX_SEQ_NUM + 1);
                memcpy(&fileLengthSeg.data, &file_length_in_bytes, sizeof(uint32_t));
                packet_queue.emplace(Packet(fileLengthSeg));

                /* packetize the file content and push it into packet_queue */
                // cout << "File Size: " << file_length_in_bytes << endl;
                int total_num_packet = ceil(file_length_in_bytes/(double)SEGMENT_PAYLOAD_SIZE);
                // cout << "Total number of packet needed: " << total_num_packet << endl;
                for (int i = 0; i < total_num_packet; ++i) {
                    struct RDTSegment fileDataSeg;
                    memset(&fileDataSeg, 0, sizeof(struct RDTSegment));
                    fileDataSeg.header.seqNum = server_sn;
                    server_sn = (server_sn + SEGMENT_PAYLOAD_SIZE) % (MAX_SEQ_NUM + 1);
                    memcpy(&fileDataSeg.data, requestedFile_c + i * SEGMENT_PAYLOAD_SIZE, SEGMENT_PAYLOAD_SIZE);
                    packet_queue.emplace(Packet(fileDataSeg));
                }
            } else if (server_state == ESTABLISHED && isAck(&recvSeg.header)) {
                for (auto& p : send_buffer) {
                    struct RDTSegment s = p.segment();
                    if (!p.isReceived() && recvSeg.header.ackNum == generateAck(&s)) {
                        p.markReceived();
                        while (!send_buffer.empty() && send_buffer.front().isReceived())
                            send_buffer.pop_front();
                    }
                }
            } else if (server_state == FIN_WAIT_1 && isAck(&recvSeg.header)) {
                assert(send_buffer.size() == 1); /* only one FIN packet should be in send_buffer at this moment */
                assert(recvSeg.header.ackNum == send_buffer.front().segment().header.seqNum); /* the ack received should ack the FIN */
                send_buffer.pop_front();
                server_state = FIN_WAIT_2;
            } else if (server_state == FIN_WAIT_2 && isFin(&recvSeg.header)) {
                server_state = TIME_WAIT;
                struct RDTSegment FINACKSeg;
                memset(&FINACKSeg, 0, sizeof(struct RDTSegment));
                FINACKSeg.header.ackNum = generateAck(&recvSeg);
                setAck(&FINACKSeg.header);
                time_wait_start_time = monotonic_clock::now();
                // cout << "Time wait starts..." << endl;
                sendToWithPrint(sockfd, &FINACKSeg, sizeof(struct RDTSegment), (struct sockaddr *) &clientAddress, sizeof(clientAddress), false);  
            } else if (server_state == TIME_WAIT && isFin(&recvSeg.header)) {
                struct RDTSegment FINACKSeg;
                memset(&FINACKSeg, 0, sizeof(struct RDTSegment));
                FINACKSeg.header.ackNum = generateAck(&recvSeg);
                setAck(&FINACKSeg.header);
                sendToWithPrint(sockfd, &FINACKSeg, sizeof(struct RDTSegment), (struct sockaddr *) &clientAddress, sizeof(clientAddress), false);  
            } else {
                // do nothing
            }
        }

        int nextPacketNumIter = send_buffer.size();

        /* push packets need to send into the send_buffer */
        while (send_buffer.size() < send_window_size && !packet_queue.empty()) {
            send_buffer.emplace_back(packet_queue.front());
            packet_queue.pop();
        }

        /* when finish sending the file, initialize the FIN/ACK closing procedure */
        if (server_state == ESTABLISHED && send_buffer.empty()) {
            server_state = FIN_WAIT_1;
            struct RDTSegment FINSeg;
            memset(&FINSeg, 0, sizeof(struct RDTSegment));
            setFin(&FINSeg.header);
            FINSeg.header.seqNum = server_sn;
            server_sn = (server_sn + 1) % (MAX_SEQ_NUM + 1);
            send_buffer.emplace_back(Packet(FINSeg));
        }

        /* retransmission due to timeout */
        for (auto it = send_buffer.begin(); it < send_buffer.begin() + nextPacketNumIter; ++it) {
            if (!it->isReceived() && duration_cast<milliseconds>(monotonic_clock::now() - it->sentTime()) > RETRANSMISSION_TIME_OUT) {
                it->updateSentTime();
                struct RDTSegment s = it->segment();
                sendToWithPrint(sockfd, &s, sizeof(struct RDTSegment), (struct sockaddr *) &clientAddress, sizeof(clientAddress), true);  
            }
        }

        /* sending for the first time */
        for (auto it = send_buffer.begin() + nextPacketNumIter; it < send_buffer.end(); ++it) {
            it->updateSentTime();
            struct RDTSegment s = it->segment();
            sendToWithPrint(sockfd, &s, sizeof(struct RDTSegment), (struct sockaddr *) &clientAddress, sizeof(clientAddress), false);  
        }

        /* time wait for closing */
        if (server_state == TIME_WAIT && duration_cast<milliseconds>(monotonic_clock::now() - time_wait_start_time) > TIME_WAIT_DURATION) {
            // cout << "Time wait finished!" << endl;
            server_state = LISTEN;
        }
    }

    return 0; // never reached
}
