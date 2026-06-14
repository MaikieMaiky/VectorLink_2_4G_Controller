#ifndef VECTORLINK_BATTERY_MONITOR_HPP
#define VECTORLINK_BATTERY_MONITOR_HPP

#include "app_state.hpp"

#include <cstdint>

namespace vectorlink
{

class BatteryMonitor final
{
public:
  BatteryState Update(uint16_t raw_adc) const;
  static uint16_t ConvertToMillivolts(uint16_t raw_adc);
};

} // namespace vectorlink

#endif // VECTORLINK_BATTERY_MONITOR_HPP
