#ifndef VECTORLINK_BATTERY_MONITOR_HPP
#define VECTORLINK_BATTERY_MONITOR_HPP

#include "app_state.hpp"

#include <cstdint>

namespace vectorlink
{

/** @brief Converts the divided ADC sample into voltage, percentage, and warning level. */
class BatteryMonitor final
{
public:
  /** Produces a complete battery state from one raw ADC sample. */
  BatteryState Update(uint16_t raw_adc) const;

  /** Converts a raw ADC sample to battery-side millivolts using the configured divider. */
  static uint16_t ConvertToMillivolts(uint16_t raw_adc);
};

} // namespace vectorlink

#endif // VECTORLINK_BATTERY_MONITOR_HPP
