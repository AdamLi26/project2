#ifndef PACKET_MODULE
#define PACKET_MODULE

#include <stdint.h>
#include "RDTSegment.h"
#include <time.h>


class PacketModule {
public:
  PacketModule(uint16_t seq_num, uint16_t flag);

private:
  RDTSegment m_segment;
  bool m_acked = false;
  uint16_t m_seq_num;
  timespec m_send_time;
};





#endif //PACKET_MODULE
