#include "board_gpio.hpp"

extern "C"
{
#include "main.h"
}

namespace vectorlink
{
namespace
{

struct ButtonPin
{
  GPIO_TypeDef* port;
  uint16_t pin;
};

const ButtonPin kButtonPins[] = {
    {KEY1_GPIO_Port, KEY1_Pin},   {KEY2_GPIO_Port, KEY2_Pin},   {KEY3_GPIO_Port, KEY3_Pin},
    {KEY4_GPIO_Port, KEY4_Pin},   {KEY5_GPIO_Port, KEY5_Pin},   {KEY6_GPIO_Port, KEY6_Pin},
    {KEY7_GPIO_Port, KEY7_Pin},   {KEY8_GPIO_Port, KEY8_Pin},   {KEY9_GPIO_Port, KEY9_Pin},
    {KEY10_GPIO_Port, KEY10_Pin}, {KEY11_GPIO_Port, KEY11_Pin}, {KEY12_GPIO_Port, KEY12_Pin},
};

} // namespace

uint16_t BoardGpio::ReadPressedButtons()
{
  uint16_t result = 0;
  for (uint8_t index = 0; index < 12; ++index)
  {
    if (HAL_GPIO_ReadPin(kButtonPins[index].port, kButtonPins[index].pin) == GPIO_PIN_RESET)
    {
      result |= static_cast<uint16_t>(1U << index);
    }
  }
  return result;
}

} // namespace vectorlink
