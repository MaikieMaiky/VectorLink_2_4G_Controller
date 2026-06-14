#include "link_quality.hpp"

namespace vectorlink
{

void LinkQuality::Record(bool success, uint32_t now_ms)
{
  history_ = (history_ << 1U) | (success ? 1U : 0U);
  if (sample_count_ < 32)
  {
    ++sample_count_;
  }
  if (success)
  {
    last_success_ms_ = now_ms;
    has_success_ = true;
  }
}

uint8_t LinkQuality::SuccessRate() const
{
  if (sample_count_ == 0)
  {
    return 0;
  }

  uint32_t value = history_;
  uint8_t successes = 0;
  for (uint8_t index = 0; index < sample_count_; ++index)
  {
    successes += static_cast<uint8_t>(value & 1U);
    value >>= 1U;
  }
  return static_cast<uint8_t>((successes * 100U) / sample_count_);
}

bool LinkQuality::IsConnected(uint32_t now_ms, uint32_t timeout_ms) const
{
  return has_success_ && static_cast<uint32_t>(now_ms - last_success_ms_) <= timeout_ms;
}

} // namespace vectorlink
