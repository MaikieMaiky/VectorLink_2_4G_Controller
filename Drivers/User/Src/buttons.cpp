#include "buttons.hpp"

namespace vectorlink
{

ButtonState Buttons::Update(uint16_t raw_pressed_mask)
{
  const uint16_t previous = stable_mask_;

  for (uint8_t index = 0; index < kButtonCount; ++index)
  {
    const uint16_t bit = static_cast<uint16_t>(1U << index);
    const bool raw_pressed = (raw_pressed_mask & bit) != 0U;
    const bool stable_pressed = (stable_mask_ & bit) != 0U;

    if (raw_pressed == stable_pressed)
    {
      counters_[index] = 0;
      continue;
    }

    if (++counters_[index] >= kStableSamples)
    {
      stable_mask_ ^= bit;
      counters_[index] = 0;
    }
  }

  return {
      stable_mask_,
      static_cast<uint16_t>(stable_mask_ & ~previous),
      static_cast<uint16_t>(previous & ~stable_mask_),
  };
}

} // namespace vectorlink
