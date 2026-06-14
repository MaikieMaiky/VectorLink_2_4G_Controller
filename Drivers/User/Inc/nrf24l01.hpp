#ifndef VECTORLINK_NRF24L01_HPP
#define VECTORLINK_NRF24L01_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace vectorlink
{

enum class RadioTransmitResult : uint8_t
{
  Pending,
  Success,
  MaximumRetries,
  Timeout,
  BusError,
};

struct Nrf24Config
{
  uint8_t channel;
  std::array<uint8_t, 5> address;
};

class Nrf24l01 final
{
public:
  static constexpr size_t kMaximumPayloadSize = 32;

  bool Initialize(const Nrf24Config& config);
  bool StartTransmit(const uint8_t* payload, uint8_t length);
  RadioTransmitResult CompleteTransmit(uint8_t* received_payload, uint8_t& received_length);
  RadioTransmitResult CheckTransmitTimeout(uint32_t now_ms, uint32_t timeout_ms);
  bool EnterReceiveMode();
  bool ReadPayload(uint8_t* payload, uint8_t& length);

private:
  static constexpr uint8_t kStatusRxReady = 1U << 6U;
  static constexpr uint8_t kStatusTxSuccess = 1U << 5U;
  static constexpr uint8_t kStatusMaximumRetries = 1U << 4U;

  uint8_t Transfer(uint8_t value, bool& success);
  uint8_t Command(uint8_t command);
  uint8_t ReadRegister(uint8_t reg, bool& success);
  bool ReadRegisters(uint8_t reg, uint8_t* data, uint8_t length);
  bool WriteRegister(uint8_t reg, uint8_t value);
  bool WriteRegisters(uint8_t reg, const uint8_t* data, uint8_t length);
  bool WritePayload(const uint8_t* payload, uint8_t length);
  void SetChipEnable(bool enabled);
  void SetChipSelect(bool selected);
  void FlushTx();
  void FlushRx();
  void ClearStatus(uint8_t flags);

  Nrf24Config config_ = {};
  uint32_t transmit_started_ms_ = 0;
  bool initialized_ = false;
  bool transmit_pending_ = false;
};

} // namespace vectorlink

#endif // VECTORLINK_NRF24L01_HPP
