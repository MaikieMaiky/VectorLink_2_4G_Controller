#include "nrf24l01.hpp"

extern "C"
{
#include "main.h"
#include "spi.h"
}

namespace vectorlink
{
namespace
{

constexpr uint8_t kCommandReadRegister = 0x00;
constexpr uint8_t kCommandWriteRegister = 0x20;
constexpr uint8_t kCommandReadPayload = 0x61;
constexpr uint8_t kCommandWritePayload = 0xA0;
constexpr uint8_t kCommandFlushTx = 0xE1;
constexpr uint8_t kCommandFlushRx = 0xE2;
constexpr uint8_t kCommandActivate = 0x50;
constexpr uint8_t kCommandReadPayloadWidth = 0x60;
constexpr uint8_t kCommandNop = 0xFF;

constexpr uint8_t kRegisterConfig = 0x00;
constexpr uint8_t kRegisterEnableAutoAck = 0x01;
constexpr uint8_t kRegisterEnableRxAddress = 0x02;
constexpr uint8_t kRegisterAddressWidth = 0x03;
constexpr uint8_t kRegisterRetransmit = 0x04;
constexpr uint8_t kRegisterChannel = 0x05;
constexpr uint8_t kRegisterRfSetup = 0x06;
constexpr uint8_t kRegisterStatus = 0x07;
constexpr uint8_t kRegisterRxAddress0 = 0x0A;
constexpr uint8_t kRegisterTxAddress = 0x10;
constexpr uint8_t kRegisterRxPayloadWidth0 = 0x11;
constexpr uint8_t kRegisterDynamicPayload = 0x1C;
constexpr uint8_t kRegisterFeature = 0x1D;

constexpr uint8_t kConfigCrcTwoBytes = 0x0C;
constexpr uint8_t kConfigPowerUp = 0x02;
constexpr uint8_t kConfigPrimaryRx = 0x01;
constexpr uint8_t kFeatureAckPayloadAndDynamicLength = 0x06;
constexpr uint32_t kSpiTimeoutMs = 10;

} // namespace

bool Nrf24l01::Initialize(const Nrf24Config& config)
{
  // Keep CE low while changing configuration; high CE would place the radio in an active mode.
  config_ = config;
  SetChipEnable(false);
  SetChipSelect(false);
  HAL_Delay(5);

  bool success = true;
  const uint8_t original_channel = ReadRegister(kRegisterChannel, success);
  if (!success || original_channel == 0xFFU)
  {
    return false;
  }

  success =
      WriteRegister(kRegisterConfig, kConfigCrcTwoBytes) &&
      WriteRegister(kRegisterEnableAutoAck, 0x01) &&
      WriteRegister(kRegisterEnableRxAddress, 0x01) && WriteRegister(kRegisterAddressWidth, 0x03) &&
      WriteRegister(kRegisterRetransmit, 0x2F) &&
      WriteRegister(kRegisterChannel, config_.channel) && WriteRegister(kRegisterRfSetup, 0x06) &&
      WriteRegisters(kRegisterTxAddress, config_.address.data(), config_.address.size()) &&
      WriteRegisters(kRegisterRxAddress0, config_.address.data(), config_.address.size()) &&
      WriteRegister(kRegisterRxPayloadWidth0, 24);
  if (!success)
  {
    return false;
  }

  // Some NRF24L01-compatible devices require ACTIVATE 0x73 before FEATURE can be written.
  WriteRegister(kRegisterFeature, kFeatureAckPayloadAndDynamicLength);
  bool feature_read_ok = true;
  if (ReadRegister(kRegisterFeature, feature_read_ok) != kFeatureAckPayloadAndDynamicLength)
  {
    SetChipSelect(true);
    bool transfer_ok = true;
    Transfer(kCommandActivate, transfer_ok);
    Transfer(0x73, transfer_ok);
    SetChipSelect(false);
    if (!transfer_ok || !WriteRegister(kRegisterFeature, kFeatureAckPayloadAndDynamicLength))
    {
      return false;
    }
  }

  if (!WriteRegister(kRegisterDynamicPayload, 0x01))
  {
    return false;
  }

  FlushTx();
  FlushRx();
  ClearStatus(kStatusRxReady | kStatusTxSuccess | kStatusMaximumRetries);
  if (!WriteRegister(kRegisterConfig, kConfigCrcTwoBytes | kConfigPowerUp))
  {
    return false;
  }

  HAL_Delay(2);
  initialized_ = true;
  return true;
}

bool Nrf24l01::StartTransmit(const uint8_t* payload, uint8_t length)
{
  if (!initialized_ || transmit_pending_ || payload == nullptr || length == 0 ||
      length > kMaximumPayloadSize)
  {
    return false;
  }

  SetChipEnable(false);
  if (!WriteRegister(kRegisterConfig, kConfigCrcTwoBytes | kConfigPowerUp))
  {
    return false;
  }
  ClearStatus(kStatusRxReady | kStatusTxSuccess | kStatusMaximumRetries);
  FlushTx();
  if (!WritePayload(payload, length))
  {
    return false;
  }

  // A CE pulse longer than 10 us starts one TX transaction in primary transmit mode.
  SetChipEnable(true);
  for (volatile uint32_t delay = 0; delay < 1000; ++delay)
  {
    __NOP();
  }
  SetChipEnable(false);

  transmit_started_ms_ = HAL_GetTick();
  transmit_pending_ = true;
  return true;
}

RadioTransmitResult Nrf24l01::CompleteTransmit(uint8_t* received_payload, uint8_t& received_length)
{
  received_length = 0;
  if (!transmit_pending_)
  {
    return RadioTransmitResult::Pending;
  }

  bool success = true;
  const uint8_t status = ReadRegister(kRegisterStatus, success);
  if (!success)
  {
    transmit_pending_ = false;
    return RadioTransmitResult::BusError;
  }

  // MAX_RT leaves the failed payload in TX FIFO, so it must be flushed before the next report.
  if ((status & kStatusMaximumRetries) != 0U)
  {
    ClearStatus(kStatusMaximumRetries);
    FlushTx();
    transmit_pending_ = false;
    return RadioTransmitResult::MaximumRetries;
  }

  if ((status & kStatusTxSuccess) == 0U)
  {
    return RadioTransmitResult::Pending;
  }

  ClearStatus(kStatusTxSuccess);
  // RX_DR together with TX_DS indicates that the hardware ACK contained an ACK payload.
  if ((status & kStatusRxReady) != 0U && received_payload != nullptr)
  {
    ReadPayload(received_payload, received_length);
  }
  transmit_pending_ = false;
  return RadioTransmitResult::Success;
}

RadioTransmitResult Nrf24l01::CheckTransmitTimeout(uint32_t now_ms, uint32_t timeout_ms)
{
  if (!transmit_pending_ || static_cast<uint32_t>(now_ms - transmit_started_ms_) < timeout_ms)
  {
    return RadioTransmitResult::Pending;
  }
  SetChipEnable(false);
  FlushTx();
  ClearStatus(kStatusTxSuccess | kStatusMaximumRetries);
  transmit_pending_ = false;
  return RadioTransmitResult::Timeout;
}

bool Nrf24l01::EnterReceiveMode()
{
  if (!initialized_)
  {
    return false;
  }
  SetChipEnable(false);
  if (!WriteRegister(kRegisterConfig, kConfigCrcTwoBytes | kConfigPowerUp | kConfigPrimaryRx))
  {
    return false;
  }
  ClearStatus(kStatusRxReady);
  SetChipEnable(true);
  return true;
}

bool Nrf24l01::ReadPayload(uint8_t* payload, uint8_t& length)
{
  length = 0;
  if (payload == nullptr)
  {
    return false;
  }

  SetChipSelect(true);
  bool success = true;
  Transfer(kCommandReadPayloadWidth, success);
  const uint8_t payload_length = Transfer(kCommandNop, success);
  SetChipSelect(false);
  // An invalid dynamic width is explicitly specified to require flushing RX FIFO.
  if (!success || payload_length == 0 || payload_length > kMaximumPayloadSize)
  {
    FlushRx();
    ClearStatus(kStatusRxReady);
    return false;
  }

  SetChipSelect(true);
  Transfer(kCommandReadPayload, success);
  for (uint8_t index = 0; index < payload_length; ++index)
  {
    payload[index] = Transfer(kCommandNop, success);
  }
  SetChipSelect(false);
  ClearStatus(kStatusRxReady);
  length = success ? payload_length : 0;
  return success;
}

uint8_t Nrf24l01::Transfer(uint8_t value, bool& success)
{
  // SPI transactions are blocking but bounded; RadioTask is the only caller and bus owner.
  uint8_t received = 0;
  if (HAL_SPI_TransmitReceive(&hspi2, &value, &received, 1, kSpiTimeoutMs) != HAL_OK)
  {
    success = false;
  }
  return received;
}

uint8_t Nrf24l01::Command(uint8_t command)
{
  SetChipSelect(true);
  bool success = true;
  const uint8_t status = Transfer(command, success);
  SetChipSelect(false);
  return success ? status : 0xFF;
}

uint8_t Nrf24l01::ReadRegister(uint8_t reg, bool& success)
{
  SetChipSelect(true);
  Transfer(kCommandReadRegister | (reg & 0x1FU), success);
  const uint8_t value = Transfer(kCommandNop, success);
  SetChipSelect(false);
  return value;
}

bool Nrf24l01::ReadRegisters(uint8_t reg, uint8_t* data, uint8_t length)
{
  bool success = true;
  SetChipSelect(true);
  Transfer(kCommandReadRegister | (reg & 0x1FU), success);
  for (uint8_t index = 0; index < length; ++index)
  {
    data[index] = Transfer(kCommandNop, success);
  }
  SetChipSelect(false);
  return success;
}

bool Nrf24l01::WriteRegister(uint8_t reg, uint8_t value)
{
  return WriteRegisters(reg, &value, 1);
}

bool Nrf24l01::WriteRegisters(uint8_t reg, const uint8_t* data, uint8_t length)
{
  bool success = true;
  SetChipSelect(true);
  Transfer(kCommandWriteRegister | (reg & 0x1FU), success);
  for (uint8_t index = 0; index < length; ++index)
  {
    Transfer(data[index], success);
  }
  SetChipSelect(false);
  return success;
}

bool Nrf24l01::WritePayload(const uint8_t* payload, uint8_t length)
{
  bool success = true;
  SetChipSelect(true);
  Transfer(kCommandWritePayload, success);
  for (uint8_t index = 0; index < length; ++index)
  {
    Transfer(payload[index], success);
  }
  SetChipSelect(false);
  return success;
}

void Nrf24l01::SetChipEnable(bool enabled)
{
  HAL_GPIO_WritePin(NRF_CE_GPIO_Port, NRF_CE_Pin, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Nrf24l01::SetChipSelect(bool selected)
{
  HAL_GPIO_WritePin(NRF_CSN_GPIO_Port, NRF_CSN_Pin, selected ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void Nrf24l01::FlushTx()
{
  Command(kCommandFlushTx);
}

void Nrf24l01::FlushRx()
{
  Command(kCommandFlushRx);
}

void Nrf24l01::ClearStatus(uint8_t flags)
{
  WriteRegister(kRegisterStatus, flags);
}

} // namespace vectorlink
