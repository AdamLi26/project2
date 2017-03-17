#ifndef PACKET_INCLUDED
#define PACKET_INCLUDED

#include <cstdint>
#include <chrono>
#include "RDTSegment.h"

class Packet {
public:
	Packet();
 	Packet(struct RDTSegment s);
 	
 	struct RDTSegment segment() const;
 	bool isReceived() const;
 	std::chrono::monotonic_clock::time_point sentTime() const;

 	void markReceived();
	void updateSentTime();

private:
  	struct RDTSegment m_segment;
  	bool m_isReceived;
  	std::chrono::monotonic_clock::time_point m_sentTime;
};


#endif
