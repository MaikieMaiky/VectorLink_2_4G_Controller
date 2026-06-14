#ifndef VECTORLINK_LINK_QUALITY_HPP
#define VECTORLINK_LINK_QUALITY_HPP

#include <cstdint>

namespace vectorlink
{

class LinkQuality final
{
public:
  void Record(bool success, uint32_t now_ms);
  uint8_t SuccessRate() const;
  bool IsConnected(uint32_t now_ms, uint32_t timeout_ms) const;

private:
  uint32_t history_ = 0;
  uint8_t sample_count_ = 0;
  uint32_t last_success_ms_ = 0;
  bool has_success_ = false;
};

} // namespace vectorlink

#endif // VECTORLINK_LINK_QUALITY_HPP
