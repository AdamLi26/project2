
#include "Packet.h"
#include <cstdint>
#include "RDTSegment.h"

using namespace std::chrono;

Packet::Packet() {
  m_isReceived = false;
}

Packet::Packet(struct RDTSegment s) {
  m_segment = s;
  m_isReceived = false;
  m_sentTime = monotonic_clock::now();
}

struct RDTSegment Packet::segment() const {
  return m_segment;
}

bool Packet::isReceived() const {
  return m_isReceived;
}

monotonic_clock::time_point Packet::sentTime() const {
  return m_sentTime;
}

void Packet::markReceived() {
  m_isReceived = true;
}

void Packet::updateSentTime() {
  m_sentTime = monotonic_clock::now();
}