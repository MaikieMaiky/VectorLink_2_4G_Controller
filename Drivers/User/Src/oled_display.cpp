#include "oled_display.hpp"

#include "app_config.hpp"

extern "C"
{
#include "i2c.h"
}

#include <algorithm>

namespace vectorlink
{
namespace
{

constexpr uint16_t kI2cTimeoutMs = 20;

constexpr uint8_t kBlank[5] = {};
constexpr uint8_t kDigits[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00}, {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03}, {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
};
constexpr uint8_t kLetters[26][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
};
constexpr uint8_t kMinus[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
constexpr uint8_t kColon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
constexpr uint8_t kDot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
constexpr uint8_t kPercent[5] = {0x23, 0x13, 0x08, 0x64, 0x62};
constexpr uint8_t kSlash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};

} // namespace

bool OledDisplay::Initialize()
{
  // Probe first so a disconnected display degrades gracefully instead of delaying every UI frame.
  HAL_Delay(20);
  ready_ = HAL_I2C_IsDeviceReady(&hi2c1, config::kOledAddress << 1U, 2, kI2cTimeoutMs) == HAL_OK;
  if (!ready_)
  {
    return false;
  }

  constexpr uint8_t commands[] = {0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40, 0x81, 0x7F,
                                  0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3, 0x00, 0xD5, 0x80, 0xD9,
                                  0xF1, 0xDA, 0x12, 0xDB, 0x40, 0x8D, 0x14, 0xAF};
  for (uint8_t command : commands)
  {
    if (!WriteCommand(command))
    {
      ready_ = false;
      return false;
    }
  }

  Clear();
  return UpdateScreen();
}

void OledDisplay::Render(const AppSnapshot& state)
{
  if (!ready_)
  {
    return;
  }

  Clear();
  if (state.page == 0)
  {
    DrawText(0, 0, "VL CONTROL");
    DrawText(0, 10, "LH:");
    DrawSigned(18, 10, state.input.axes[0]);
    DrawText(64, 10, "RH:");
    DrawSigned(82, 10, state.input.axes[2]);
    DrawText(0, 20, "LV:");
    DrawSigned(18, 20, state.input.axes[1]);
    DrawText(64, 20, "RV:");
    DrawSigned(82, 20, state.input.axes[3]);
    DrawText(0, 32, "KEY:");
    DrawUnsigned(30, 32, state.input.buttons, 4);
    DrawBattery(0, 44, state.input.battery);
  }
  else
  {
    DrawText(0, 0, "RADIO STATUS");
    DrawText(0, 10, state.radio.initialized ? "INIT:OK" : "INIT:FAIL");
    DrawText(64, 10, state.radio.connected ? "LINK:OK" : "LINK:LOST");
    DrawText(0, 20, "RATE:");
    DrawUnsigned(30, 20, state.radio.success_rate);
    DrawText(48, 20, "%");
    DrawText(64, 20, "SEQ:");
    DrawUnsigned(88, 20, state.radio.sequence);
    DrawText(0, 32, "TLM:");
    DrawUnsigned(24, 32, state.telemetry.acknowledged_sequence);
    DrawText(64, 32, "BAT:");
    DrawUnsigned(88, 32, state.telemetry.receiver_battery_mv);
    for (uint8_t index = 0; index < 4; ++index)
    {
      DrawSigned(static_cast<uint8_t>((index % 2U) * 64U),
                 static_cast<uint8_t>(44U + (index / 2U) * 10U), state.telemetry.values[index]);
    }
  }

  if (!UpdateScreen())
  {
    ready_ = false;
  }
}

bool OledDisplay::IsReady() const
{
  return ready_;
}

bool OledDisplay::WriteCommand(uint8_t command)
{
  uint8_t data[] = {0x00, command};
  return HAL_I2C_Master_Transmit(&hi2c1, config::kOledAddress << 1U, data, sizeof(data),
                                 kI2cTimeoutMs) == HAL_OK;
}

bool OledDisplay::UpdateScreen()
{
  // Horizontal addressing lets one 1024-byte framebuffer be streamed from column 0, page 0.
  if (!WriteCommand(0x21) || !WriteCommand(0) || !WriteCommand(kWidth - 1) || !WriteCommand(0x22) ||
      !WriteCommand(0) || !WriteCommand(7))
  {
    return false;
  }

  // Prefix each small chunk with the SSD13xx data control byte. Short chunks keep the blocking I2C
  // call bounded and avoid a second 1 KiB staging buffer.
  std::array<uint8_t, 17> transfer = {};
  transfer[0] = 0x40;
  for (uint16_t offset = 0; offset < buffer_.size(); offset += 16)
  {
    std::copy_n(buffer_.begin() + offset, 16, transfer.begin() + 1);
    if (HAL_I2C_Master_Transmit(&hi2c1, config::kOledAddress << 1U, transfer.data(),
                                transfer.size(), kI2cTimeoutMs) != HAL_OK)
    {
      return false;
    }
  }
  return true;
}

void OledDisplay::Clear()
{
  buffer_.fill(0);
}

void OledDisplay::DrawChar(uint8_t x, uint8_t y, char character)
{
  if (x + 5 >= kWidth || y + 7 >= kHeight)
  {
    return;
  }

  const uint8_t* glyph = Glyph(character);
  for (uint8_t column = 0; column < 5; ++column)
  {
    for (uint8_t row = 0; row < 7; ++row)
    {
      if ((glyph[column] & (1U << row)) != 0U)
      {
        // The controller stores eight vertical pixels per byte, arranged in eight 128-byte pages.
        const uint16_t index = static_cast<uint16_t>((y + row) / 8U) * kWidth + x + column;
        buffer_[index] |= static_cast<uint8_t>(1U << ((y + row) % 8U));
      }
    }
  }
}

void OledDisplay::DrawText(uint8_t x, uint8_t y, const char* text)
{
  while (*text != '\0' && x + 5 < kWidth)
  {
    DrawChar(x, y, *text++);
    x = static_cast<uint8_t>(x + 6U);
  }
}

void OledDisplay::DrawUnsigned(uint8_t x, uint8_t y, uint32_t value, uint8_t width)
{
  char digits[11] = {};
  uint8_t count = 0;
  do
  {
    digits[count++] = static_cast<char>('0' + value % 10U);
    value /= 10U;
  } while (value != 0U && count < sizeof(digits));
  while (count < width && count < sizeof(digits))
  {
    digits[count++] = '0';
  }
  while (count > 0)
  {
    DrawChar(x, y, digits[--count]);
    x = static_cast<uint8_t>(x + 6U);
  }
}

void OledDisplay::DrawSigned(uint8_t x, uint8_t y, int32_t value)
{
  if (value < 0)
  {
    DrawChar(x, y, '-');
    x = static_cast<uint8_t>(x + 6U);
    value = -value;
  }
  DrawUnsigned(x, y, static_cast<uint32_t>(value));
}

void OledDisplay::DrawBattery(uint8_t x, uint8_t y, const BatteryState& battery)
{
  DrawText(x, y, "BAT:");
  DrawUnsigned(static_cast<uint8_t>(x + 24U), y, battery.voltage_mv / 1000U);
  DrawChar(static_cast<uint8_t>(x + 30U), y, '.');
  DrawUnsigned(static_cast<uint8_t>(x + 36U), y, (battery.voltage_mv % 1000U) / 10U, 2);
  DrawText(static_cast<uint8_t>(x + 54U), y, "V");
  DrawUnsigned(static_cast<uint8_t>(x + 72U), y, battery.percentage);
  DrawText(static_cast<uint8_t>(x + 90U), y, "%");
}

const uint8_t* OledDisplay::Glyph(char character)
{
  if (character >= '0' && character <= '9')
  {
    return kDigits[character - '0'];
  }
  if (character >= 'a' && character <= 'z')
  {
    character = static_cast<char>(character - 'a' + 'A');
  }
  if (character >= 'A' && character <= 'Z')
  {
    return kLetters[character - 'A'];
  }
  switch (character)
  {
    case '-':
      return kMinus;
    case ':':
      return kColon;
    case '.':
      return kDot;
    case '%':
      return kPercent;
    case '/':
      return kSlash;
    default:
      return kBlank;
  }
}

} // namespace vectorlink
