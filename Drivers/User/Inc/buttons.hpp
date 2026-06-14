#ifndef VECTORLINK_BUTTONS_HPP
#define VECTORLINK_BUTTONS_HPP

#include <array>
#include <cstdint>

namespace vectorlink
{

/** @brief Stable button levels and one-update edge events. */
struct ButtonState
{
  uint16_t current = 0;
  uint16_t pressed = 0;
  uint16_t released = 0;
};

/** @brief Debounces twelve buttons with independent consecutive-sample counters. */
class Buttons final
{
public:
  /** Updates the filter from an active-high pressed mask and returns the stable result. */
  ButtonState Update(uint16_t raw_pressed_mask);

private:
  static constexpr uint8_t kButtonCount = 12;
  static constexpr uint8_t kStableSamples = 3;

  std::array<uint8_t, kButtonCount> counters_ = {};
  uint16_t stable_mask_ = 0;
};

} // namespace vectorlink

#endif // VECTORLINK_BUTTONS_HPP
