#ifndef UTILITY_INCLUDED
#define UTILITY_INCLUDED 

#include <iostream>      // for io
#include <string>        // for string
#include <cstdlib>       // definitions of exit status
#include <cstring>

#include <RDTSegment.h>

using namespace std;

void dieWithError(const string& errorMsg) {
    cerr << errorMsg << endl;
    exit(EXIT_FAILURE);
}

// helpful functions
void toLocal(struct RDTHeader *h) {
	h->seqNum = ntohs(h->seqNum);
	h->ackNum = ntohs(h->ackNum);
	h->flag = ntohs(h->flag);
	h->recvW = ntohs(h->recvW);
}

void toNetwork(struct RDTHeader *h) {
	h->seqNum = htons(h->seqNum);
	h->ackNum = htons(h->ackNum);
	h->flag = htons(h->flag);
	h->recvW = htons(h->recvW);
}

bool isSyn(struct RDTHeader *h) {
	if (h->flag & 2) // 0000000000000010
		return true;
	return false;
}

void setSyn(struct RDTHeader *h) {
	h->flag |= 2;
}

bool isFin(struct RDTHeader *h) {
	if (h->flag & 1) // 0000000000000001
		return true;
	return false;
}

void setFin(struct RDTHeader *h) {
	h->flag |= 1;
}

uint16_t dataLength(struct RDTHeader *h) {
	return h->flag >> 6;
}

void ack(struct RDTSegment *s) {
	uint16_t dataLen = dataLength(&s->header);
	uint16_t ackNum = isSyn(&s->header) ? s->header.seqNum + 1 : s->header.seqNum + dataLen;
	if (ackNum > MAX_SEQ_NUM) 
		ackNum %= (MAX_SEQ_NUM+1);
	s->header.ackNum = ackNum;
	// memset(s->data, 0, 1016); // 1024-8 
}

void print(struct RDTSegment *s) {
	uint16_t dataLen = dataLength(&s->header);
	cout << "sequence number: " << s->header.seqNum << endl;
	cout << "ackowledgement number: " << s->header.ackNum << endl;
	cout << "data length: " << dataLen << endl;
	cout << "flag: " << (s->header.flag & 3) << endl;
	cout << "recvW: " << s->header.recvW << endl;

	char *buffer = new char[dataLen+1];
	strncpy(buffer, s->data, dataLen);
	buffer[dataLen] = '\0';
	cout << "data: " << buffer << endl;
	delete buffer;
}

#endif