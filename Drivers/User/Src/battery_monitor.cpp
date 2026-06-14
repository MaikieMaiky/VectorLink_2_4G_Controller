#include "battery_monitor.hpp"

#include "app_config.hpp"

#include <algorithm>

namespace vectorlink
{

BatteryState BatteryMonitor::Update(uint16_t raw_adc) const
{
  BatteryState state = {};
  state.voltage_mv = ConvertToMillivolts(raw_adc);

  // This linear estimate is intentionally simple and is not a chemistry-accurate state-of-charge
  // model. Real voltage calibration belongs to hardware bring-up.
  const int32_t range = config::kBatteryFullMv - config::kBatteryEmptyMv;
  const int32_t offset = static_cast<int32_t>(state.voltage_mv) - config::kBatteryEmptyMv;
  state.percentage = static_cast<uint8_t>(std::clamp((offset * 100) / range, 0, 100));

  if (state.voltage_mv <= config::kBatteryCriticalMv)
  {
    state.level = BatteryLevel::Critical;
  }
  else if (state.voltage_mv <= config::kBatteryLowMv)
  {
    state.level = BatteryLevel::Low;
  }
  else
  {
    state.level = BatteryLevel::Normal;
  }

  return state;
}

uint16_t BatteryMonitor::ConvertToMillivolts(uint16_t raw_adc)
{
  // Use 32-bit intermediates and add half the denominator to round to the nearest millivolt.
  const uint32_t numerator =
      static_cast<uint32_t>(raw_adc) * config::kAdcReferenceMv * config::kBatteryDividerNumerator;
  const uint32_t denominator =
      static_cast<uint32_t>(config::kAdcFullScale) * config::kBatteryDividerDenominator;
  return static_cast<uint16_t>((numerator + denominator / 2U) / denominator);
}

} // namespace vectorlink
