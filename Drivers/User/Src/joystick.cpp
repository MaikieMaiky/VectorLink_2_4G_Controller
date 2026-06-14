#include "joystick.hpp"

#include <algorithm>

namespace vectorlink
{

Joystick::Joystick(const std::array<JoystickAxisConfig, 4>& config) : config_(config)
{
}

std::array<int16_t, 4> Joystick::Process(const std::array<uint16_t, 4>& raw)
{
  std::array<int16_t, 4> result = {};

  for (size_t index = 0; index < raw.size(); ++index)
  {
    if (!initialized_)
    {
      filtered_[index] = raw[index];
    }
    else
    {
      // First-order integer low-pass filter: new = old + (sample - old) / 4.
      filtered_[index] += (static_cast<int32_t>(raw[index]) - filtered_[index]) / 4;
    }

    result[index] = Normalize(static_cast<uint16_t>(filtered_[index]), config_[index]);
  }

  initialized_ = true;
  return result;
}

int16_t Joystick::Normalize(uint16_t raw, const JoystickAxisConfig& config)
{
  const int32_t center = config.center;
  const int32_t lower_edge = center - config.dead_zone;
  const int32_t upper_edge = center + config.dead_zone;
  int32_t value = 0;

  if (raw < lower_edge)
  {
    // Each side has its own range because a real potentiometer center is rarely exactly halfway.
    const int32_t range = std::max<int32_t>(1, lower_edge - config.minimum);
    value = -((lower_edge - raw) * 1000) / range;
  }
  else if (raw > upper_edge)
  {
    const int32_t range = std::max<int32_t>(1, config.maximum - upper_edge);
    value = ((raw - upper_edge) * 1000) / range;
  }

  value = std::clamp<int32_t>(value, -1000, 1000);
  if (config.reversed)
  {
    value = -value;
  }

  return static_cast<int16_t>(value);
}

} // namespace vectorlink
