#ifndef VECTORLINK_APP_STATE_HPP
#define VECTORLINK_APP_STATE_HPP

#include <array>
#include <cstdint>

namespace vectorlink
{

/** @brief Coarse battery state used by the UI and warning policy. */
enum class BatteryLevel : uint8_t
{
  Normal,
  Low,
  Critical,
};

/** @brief Processed battery measurement expressed in physical units. */
struct BatteryState
{
  uint16_t voltage_mv = 0;
  uint8_t percentage = 0;
  BatteryLevel level = BatteryLevel::Critical;
};

/** @brief Latest debounced and normalized controller input snapshot. */
struct InputState
{
  std::array<int16_t, 4> axes = {};
  uint16_t buttons = 0;
  uint16_t pressed = 0;
  uint16_t released = 0;
  BatteryState battery = {};
};

/** @brief Last valid telemetry payload received through an ACK payload. */
struct TelemetryState
{
  bool valid = false;
  uint16_t acknowledged_sequence = 0;
  uint16_t receiver_battery_mv = 0;
  std::array<int16_t, 4> values = {};
  uint16_t flags = 0;
};

/** @brief Radio health and cumulative communication counters. */
struct RadioState
{
  bool initialized = false;
  bool connected = false;
  uint8_t success_rate = 0;
  uint16_t sequence = 0;
  uint32_t transmitted = 0;
  uint32_t succeeded = 0;
  uint32_t failed = 0;
  uint32_t received = 0;
};

/** @brief Complete immutable-by-convention view copied between application tasks. */
struct AppSnapshot
{
  InputState input = {};
  RadioState radio = {};
  TelemetryState telemetry = {};
  uint8_t page = 0;
  bool oled_ready = false;
};

} // namespace vectorlink

#endif // VECTORLINK_APP_STATE_HPP
