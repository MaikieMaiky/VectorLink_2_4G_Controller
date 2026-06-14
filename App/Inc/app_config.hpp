#ifndef VECTORLINK_APP_CONFIG_HPP
#define VECTORLINK_APP_CONFIG_HPP

#include <array>
#include <cstdint>

namespace vectorlink::config
{

constexpr uint32_t kInputPeriodMs = 10;
constexpr uint32_t kRadioPeriodMs = 20;
constexpr uint32_t kUiUpdatePeriodMs = 10;
constexpr uint32_t kDisplayPeriodMs = 100;
constexpr uint32_t kLinkLostTimeoutMs = 500;

constexpr uint16_t kAdcFullScale = 4095;
constexpr uint16_t kAdcReferenceMv = 3300;
constexpr uint16_t kJoystickCenter = 2048;
constexpr uint16_t kJoystickMinimum = 0;
constexpr uint16_t kJoystickMaximum = 4095;
constexpr uint16_t kJoystickDeadZone = 100;
constexpr std::array<bool, 4> kJoystickReversed = {false, false, false, false};

constexpr uint16_t kBatteryEmptyMv = 3300;
constexpr uint16_t kBatteryFullMv = 4200;
constexpr uint16_t kBatteryLowMv = 3500;
constexpr uint16_t kBatteryCriticalMv = 3300;
constexpr uint8_t kBatteryDividerNumerator = 2;
constexpr uint8_t kBatteryDividerDenominator = 1;

constexpr uint8_t kOledAddress = 0x3C;
constexpr uint8_t kRadioChannel = 76;
constexpr std::array<uint8_t, 5> kRadioAddress = {0x56, 0x4C, 0x32, 0x34, 0x01};

} // namespace vectorlink::config

#endif // VECTORLINK_APP_CONFIG_HPP
