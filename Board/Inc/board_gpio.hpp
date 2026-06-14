#ifndef VECTORLINK_BOARD_GPIO_HPP
#define VECTORLINK_BOARD_GPIO_HPP

#include <cstdint>

namespace vectorlink
{

/** @brief Converts the board's active-low key GPIOs into an active-high bit mask. */
class BoardGpio final
{
public:
  /** Returns bit 0..11 for currently pressed K1..K12. */
  static uint16_t ReadPressedButtons();
};

} // namespace vectorlink

#endif // VECTORLINK_BOARD_GPIO_HPP
