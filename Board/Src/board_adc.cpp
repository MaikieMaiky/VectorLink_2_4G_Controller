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

  // Circular DMA updates memory without CPU intervention; callbacks are not used.
  __HAL_DMA_DISABLE_IT(hadc1.DMA_Handle, DMA_IT_HT | DMA_IT_TC);
  return true;
}

BoardAdc::Snapshot BoardAdc::ReadSnapshot()
{
  Snapshot first = {};
  Snapshot second = {};

  // DMA is not stopped. Two matching reads avoid returning a partly updated scan.
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
