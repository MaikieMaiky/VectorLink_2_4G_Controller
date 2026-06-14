#ifndef VECTORLINK_JOYSTICK_HPP
#define VECTORLINK_JOYSTICK_HPP

#include <array>
#include <cstdint>

namespace vectorlink
{

struct JoystickAxisConfig
{
  uint16_t minimum;
  uint16_t center;
  uint16_t maximum;
  uint16_t dead_zone;
  bool reversed;
};

class Joystick final
{
public:
  explicit Joystick(const std::array<JoystickAxisConfig, 4>& config);

  std::array<int16_t, 4> Process(const std::array<uint16_t, 4>& raw);
  static int16_t Normalize(uint16_t raw, const JoystickAxisConfig& config);

private:
  std::array<JoystickAxisConfig, 4> config_;
  std::array<int32_t, 4> filtered_ = {};
  bool initialized_ = false;
};

} // namespace vectorlink

#endif // VECTORLINK_JOYSTICK_HPP
