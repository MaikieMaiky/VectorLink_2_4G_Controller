#ifndef VECTORLINK_LINK_QUALITY_HPP
#define VECTORLINK_LINK_QUALITY_HPP

#include <cstdint>

namespace vectorlink
{

/**
 * @brief Tracks a 32-transmission rolling success window and the last successful ACK.
 *
 * The class contains only value state and is independent of HAL and FreeRTOS, which makes it
 * suitable for host-side tests.
 */
class LinkQuality final
{
public:
  /** Records one completed transmission attempt. */
  void Record(bool success, uint32_t now_ms);

  /** Returns successful samples as an integer percentage in the range 0..100. */
  uint8_t SuccessRate() const;

  /** Returns true when at least one ACK was received and it is not older than the timeout. */
  bool IsConnected(uint32_t now_ms, uint32_t timeout_ms) const;

private:
  uint32_t history_ = 0;
  uint8_t sample_count_ = 0;
  uint32_t last_success_ms_ = 0;
  bool has_success_ = false;
};

} // namespace vectorlink

#endif // VECTORLINK_LINK_QUALITY_HPP
