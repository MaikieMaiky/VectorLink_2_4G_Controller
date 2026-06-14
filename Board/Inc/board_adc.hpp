#ifndef VECTORLINK_BOARD_ADC_HPP
#define VECTORLINK_BOARD_ADC_HPP

#include <array>
#include <cstdint>

namespace vectorlink
{

/** @brief Owns the board-specific ADC1 circular-DMA acquisition. */
class BoardAdc final
{
public:
  static constexpr size_t kChannelCount = 5;
  using Snapshot = std::array<uint16_t, kChannelCount>;

  /** Calibrates ADC1 and starts the five-channel circular DMA transfer. */
  static bool Initialize();

  /** Returns channels in the fixed order LH, LV, RH, RV, and battery sense. */
  static Snapshot ReadSnapshot();

private:
  static volatile uint16_t samples_[kChannelCount];
};

} // namespace vectorlink

#endif // VECTORLINK_BOARD_ADC_HPP
