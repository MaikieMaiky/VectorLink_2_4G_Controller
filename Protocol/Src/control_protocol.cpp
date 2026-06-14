#include "control_protocol.hpp"

namespace vectorlink::protocol
{
namespace
{

void WriteU16(Packet& packet, size_t offset, uint16_t value)
{
  // Explicit byte writes make the wire format independent of CPU endianness and object padding.
  packet[offset] = static_cast<uint8_t>(value & 0xFFU);
  packet[offset + 1] = static_cast<uint8_t>(value >> 8U);
}

uint16_t ReadU16(const Packet& packet, size_t offset)
{
  return static_cast<uint16_t>(packet[offset] | (static_cast<uint16_t>(packet[offset + 1]) << 8U));
}

void WriteHeader(Packet& packet, MessageType type, uint16_t sequence)
{
  packet.fill(0);
  packet[0] = 'V';
  packet[1] = 'L';
  packet[2] = kVersion;
  packet[3] = static_cast<uint8_t>(type);
  WriteU16(packet, 4, sequence);
}

void FinishPacket(Packet& packet)
{
  // CRC bytes are excluded from their own input range.
  WriteU16(packet, 22, CalculateCrc16(packet.data(), 22));
}

bool HasValidHeaderAndCrc(const Packet& packet, MessageType type)
{
  return packet[0] == 'V' && packet[1] == 'L' && packet[2] == kVersion &&
         packet[3] == static_cast<uint8_t>(type) &&
         ReadU16(packet, 22) == CalculateCrc16(packet.data(), 22);
}

} // namespace

Packet EncodeControl(const ControlData& data)
{
  Packet packet = {};
  WriteHeader(packet, MessageType::Control, data.sequence);

  for (size_t index = 0; index < data.axes.size(); ++index)
  {
    // The cast preserves the two's-complement bit pattern of a negative signed axis value.
    WriteU16(packet, 6 + index * 2, static_cast<uint16_t>(data.axes[index]));
  }

  WriteU16(packet, 14, data.buttons);
  WriteU16(packet, 16, data.battery_mv);
  WriteU16(packet, 18, data.flags);
  WriteU16(packet, 20, data.uptime_seconds);
  FinishPacket(packet);
  return packet;
}

Packet EncodeTelemetry(const TelemetryState& data, uint16_t sequence)
{
  Packet packet = {};
  WriteHeader(packet, MessageType::Telemetry, sequence);
  WriteU16(packet, 6, data.acknowledged_sequence);
  WriteU16(packet, 8, data.receiver_battery_mv);

  for (size_t index = 0; index < data.values.size(); ++index)
  {
    WriteU16(packet, 10 + index * 2, static_cast<uint16_t>(data.values[index]));
  }

  WriteU16(packet, 18, data.flags);
  FinishPacket(packet);
  return packet;
}

bool DecodeTelemetry(const Packet& packet, TelemetryState& data)
{
  if (!HasValidHeaderAndCrc(packet, MessageType::Telemetry))
  {
    return false;
  }

  data.valid = true;
  data.acknowledged_sequence = ReadU16(packet, 6);
  data.receiver_battery_mv = ReadU16(packet, 8);
  for (size_t index = 0; index < data.values.size(); ++index)
  {
    data.values[index] = static_cast<int16_t>(ReadU16(packet, 10 + index * 2));
  }
  data.flags = ReadU16(packet, 18);
  return true;
}

uint16_t CalculateCrc16(const uint8_t* data, size_t length)
{
  // CRC-16/CCITT-FALSE: polynomial 0x1021, initial 0xFFFF, no reflection or final XOR.
  uint16_t crc = 0xFFFFU;
  for (size_t index = 0; index < length; ++index)
  {
    crc ^= static_cast<uint16_t>(data[index]) << 8U;
    for (uint8_t bit = 0; bit < 8; ++bit)
    {
      crc = (crc & 0x8000U) != 0U ? static_cast<uint16_t>((crc << 1U) ^ 0x1021U)
                                  : static_cast<uint16_t>(crc << 1U);
    }
  }
  return crc;
}

} // namespace vectorlink::protocol
