#ifndef VECTORLINK_BOARD_GPIO_HPP
#define VECTORLINK_BOARD_GPIO_HPP

#include <cstdint>

namespace vectorlink
{

class BoardGpio final
{
public:
  static uint16_t ReadPressedButtons();
};

} // namespace vectorlink

#endif // VECTORLINK_BOARD_GPIO_HPP
