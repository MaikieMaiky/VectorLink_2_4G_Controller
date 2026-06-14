#ifndef VECTORLINK_CONTROL_PROTOCOL_HPP
#define VECTORLINK_CONTROL_PROTOCOL_HPP

#include "app_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vectorlink::protocol
{

constexpr size_t kPacketSize = 24;
constexpr uint8_t kVersion = 1;

enum class MessageType : uint8_t
{
  Control = 1,
  Telemetry = 2,
};

struct ControlData
{
  uint16_t sequence = 0;
  std::array<int16_t, 4> axes = {};
  uint16_t buttons = 0;
  uint16_t battery_mv = 0;
  uint16_t flags = 0;
  uint16_t uptime_seconds = 0;
};

using Packet = std::array<uint8_t, kPacketSize>;

Packet EncodeControl(const ControlData& data);
Packet EncodeTelemetry(const TelemetryState& data, uint16_t sequence);
bool DecodeTelemetry(const Packet& packet, TelemetryState& data);
uint16_t CalculateCrc16(const uint8_t* data, size_t length);

} // namespace vectorlink::protocol

#endif // VECTORLINK_CONTROL_PROTOCOL_HPP
