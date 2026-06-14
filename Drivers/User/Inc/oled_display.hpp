#ifndef VECTORLINK_OLED_DISPLAY_HPP
#define VECTORLINK_OLED_DISPLAY_HPP

#include "app_state.hpp"

#include <array>
#include <cstdint>

namespace vectorlink
{

class OledDisplay final
{
public:
  bool Initialize();
  void Render(const AppSnapshot& state);

private:
  static constexpr uint8_t kWidth = 128;
  static constexpr uint8_t kHeight = 64;
  static constexpr uint16_t kBufferSize = kWidth * kHeight / 8;

  bool WriteCommand(uint8_t command);
  bool UpdateScreen();
  void Clear();
  void DrawChar(uint8_t x, uint8_t y, char character);
  void DrawText(uint8_t x, uint8_t y, const char* text);
  void DrawUnsigned(uint8_t x, uint8_t y, uint32_t value, uint8_t width = 0);
  void DrawSigned(uint8_t x, uint8_t y, int32_t value);
  void DrawBattery(uint8_t x, uint8_t y, const BatteryState& battery);
  static const uint8_t* Glyph(char character);

  std::array<uint8_t, kBufferSize> buffer_ = {};
  bool ready_ = false;
};

} // namespace vectorlink

#endif // VECTORLINK_OLED_DISPLAY_HPP
