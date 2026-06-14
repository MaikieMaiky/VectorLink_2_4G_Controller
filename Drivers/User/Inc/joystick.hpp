#ifndef VECTORLINK_JOYSTICK_HPP
#define VECTORLINK_JOYSTICK_HPP

#include <array>
#include <cstdint>

namespace vectorlink
{

/** @brief Calibration and direction settings for one ADC joystick axis. */
struct JoystickAxisConfig
{
  uint16_t minimum;
  uint16_t center;
  uint16_t maximum;
  uint16_t dead_zone;
  bool reversed;
};

/** @brief Filters and normalizes four raw joystick axes to the range -1000..1000. */
class Joystick final
{
public:
  /**
   * @brief Copies four axis configurations into the processor.
   *
   * explicit prevents an array from being converted to Joystick unintentionally.
   */
  explicit Joystick(const std::array<JoystickAxisConfig, 4>& config);

  /** Applies the integer low-pass filter and normalization to all four axes. */
  std::array<int16_t, 4> Process(const std::array<uint16_t, 4>& raw);

  /** Normalizes one already-filtered sample; exposed for deterministic host tests. */
  static int16_t Normalize(uint16_t raw, const JoystickAxisConfig& config);

private:
  std::array<JoystickAxisConfig, 4> config_;
  std::array<int32_t, 4> filtered_ = {};
  bool initialized_ = false;
};

} // namespace vectorlink

#endif // VECTORLINK_JOYSTICK_HPP
