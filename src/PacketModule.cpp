#include "../include/RDTSegment.h"
#include <time.h>
#include "../include/PacketModule.h"
#include <stdint.h>


PacketModule::PacketModule(uint16_t seq_num, uint16_t flag) {
  m_seq_num = seq_num;

  clock_gettime(CLOCK_REALTIME, &m_send_time);
  //Note for later. To convert nanoseconds to ms:
  //include <math.h>
  // long ms = round(m_send_time.tv_nsec / 1.0e6)

  //Initialize RDTSegment and send data here. May want to move clock_gettime as
  //close to the time we actually send data
  m_segment.flag = flag;
  m_segment.ackNum = seq_num; //I believe ackNum = seq_num for SR Protocol. Please correct if wrong.

  //Read from the file into data here:
  //
  //


  clock_gettime(CLOCK_REALTIME, &m_send_time);
  //Send the data here. We should have a function that handles sending / resending the data
}
