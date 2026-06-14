#include "board_adc.hpp"

extern "C"
{
#include "adc.h"
}

namespace vectorlink
{

volatile uint16_t BoardAdc::samples_[BoardAdc::kChannelCount] = {};

bool BoardAdc::Initialize()
{
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
  {
    return false;
  }

  if (HAL_ADC_Start_DMA(&hadc1, reinterpret_cast<uint32_t*>(const_cast<uint16_t*>(samples_)),
                        kChannelCount) != HAL_OK)
  {
    return false;
  }

  // Callbacks are unused, so disabling HT/TC interrupts prevents an interrupt on every partial or
  // complete five-channel scan.
  __HAL_DMA_DISABLE_IT(hadc1.DMA_Handle, DMA_IT_HT | DMA_IT_TC);
  return true;
}

BoardAdc::Snapshot BoardAdc::ReadSnapshot()
{
  Snapshot first = {};
  Snapshot second = {};

  // DMA is not stopped. Two matching reads reduce the chance of returning a scan assembled across
  // two DMA cycles without disabling interrupts around volatile DMA-owned memory.
  for (uint8_t attempt = 0; attempt < 3; ++attempt)
  {
    for (size_t index = 0; index < kChannelCount; ++index)
    {
      first[index] = samples_[index];
    }
    for (size_t index = 0; index < kChannelCount; ++index)
    {
      second[index] = samples_[index];
    }
    if (first == second)
    {
      break;
    }
  }

  return second;
}

} // namespace vectorlink
