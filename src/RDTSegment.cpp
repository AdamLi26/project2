#include <RDTSegement.h>
#include <arpa/inet.h>
#include <cstring>

// helpful functions
bool isSyn(struct RDTSegment *segment) {
	if (ntohs(segment->header.flag) == 2)
		return true;
	return false;
}

bool isFin(struct RDTSegment *segment) {
	if (ntohs(segment->header.flag) == 1)
		return true;
	return false;
}

void ack(struct RDTSegment *segment) {
	uint16_t seqNum_local = ntohs(segment->header.seqNum);
	uint16_t data_len = strlen(segment->data);
	uint16_t ackNum_local = seqNum_local + data_lem;
	if (ackNum_local > MAXSEQNUM) 
		ackNum_local -= MAXSEQNUM;
	segment->header.ackNum = htons(ackNum_local);
	memset(segment->data, 0, 1016); // 1024-8 
}

