#include <RDTSegement.h>
#include <cstring>
#include <string>
#include <iostream>

using namespace std;

// helpful functions
void toLocal(struct RDTHeader *h) {
	h->seqNum = ntohs(h->seqNum);
	h->ackNum = ntohs(h->ackNum);
	h->flag = ntohs(h->flag);
	h->recW = ntohs(h->recW);
}

void toNetwork(struct RDTHeader *h) {
	h->seqNum = htons(h->seqNum);
	h->ackNum = htons(h->ackNum);
	h->flag = htons(h->flag);
	h->recW = htons(h->recW);
}

bool isSyn(struct RDTHeader *h) {
	if (h->flag == 2) // 0000000000000010
		return true;
	return false;
}

void setSyn(struct RDTHeader *h) {
	h->flag |= 2;
}

bool isFin(struct RDTHeader *h) {
	if (h->flag == 1) // 0000000000000001
		return true;
	return false;
}

void setFin(struct RDTHeader *h) {
	h->flag |= 1;
}

void ack(struct RDTSegment *s) {
	uint16_t data_len = strlen(s->header.data);
	uint16_t ackNum = s->header.seqNum + data_len;
	if (ackNum > MAX_SEQ_NUM) 
		ackNum %= (MAX_SEQ_NUM+1);
	memset(s->header.data, 0, 1016); // 1024-8 
}

void print(struct RDTSegment *s) {
	cout << "sequence number: " << s->header.seqNum << endl;
	cout << "ackowledgement number: " << s->header.ackNum << endl;
	cout << "flag: " << s->header.flag << endl;
	cout << "recW: " << s->header.recW << endl;
	cout << "data: " << string(s->data) << endl;
}