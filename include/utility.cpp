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

// helpful functions for RDTSegment
bool isAck(struct RDTHeader *h) {
	if (h->flag & 4) // 0000000000000100
		return true;
	return false;
}

bool isSyn(struct RDTHeader *h) {
	if (h->flag & 2) // 0000000000000010
		return true;
	return false;
}

bool isFin(struct RDTHeader *h) {
	if (h->flag & 1) // 0000000000000001
		return true;
	return false;
}

uint16_t generateAck(struct RDTSegment *s) {
	return s->header.seqNum;
}

void setAck(struct RDTHeader *h) {
	h->flag |= 4;
}

void setSyn(struct RDTHeader *h) {
	h->flag |= 2;
}

void setFin(struct RDTHeader *h) {
	h->flag |= 1;
}

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

void print(struct RDTSegment *s) {
	cout << "sequence number: " << s->header.seqNum << endl;
	cout << "ackowledgement number: " << s->header.ackNum << endl;
	cout << "flag: " << (s->header.flag & 7) << endl;
	cout << "recvW: " << s->header.recvW << endl;
	cout << "data: " << s->data << endl;
}

#endif