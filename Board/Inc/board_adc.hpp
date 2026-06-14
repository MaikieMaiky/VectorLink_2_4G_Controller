#ifndef VECTORLINK_BOARD_ADC_HPP
#define VECTORLINK_BOARD_ADC_HPP

#include <array>
#include <cstdint>

namespace vectorlink
{

class BoardAdc final
{
public:
  static constexpr size_t kChannelCount = 5;
  using Snapshot = std::array<uint16_t, kChannelCount>;

  static bool Initialize();
  static Snapshot ReadSnapshot();

private:
  static volatile uint16_t samples_[kChannelCount];
};

} // namespace vectorlink

#endif // VECTORLINK_BOARD_ADC_HPP
