#include "battery_monitor.hpp"
#include "buttons.hpp"
#include "control_protocol.hpp"
#include "joystick.hpp"
#include "link_quality.hpp"

#include <array>
#include <cstdlib>
#include <iostream>

namespace
{

void Expect(bool condition, const char* message)
{
  if (!condition)
  {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

void TestJoystick()
{
  const vectorlink::JoystickAxisConfig normal = {0, 2048, 4095, 100, false};
  const vectorlink::JoystickAxisConfig reversed = {0, 2048, 4095, 100, true};
  Expect(vectorlink::Joystick::Normalize(2048, normal) == 0, "joystick center");
  Expect(vectorlink::Joystick::Normalize(2100, normal) == 0, "joystick dead zone");
  Expect(vectorlink::Joystick::Normalize(0, normal) == -1000, "joystick minimum");
  Expect(vectorlink::Joystick::Normalize(4095, normal) == 1000, "joystick maximum");
  Expect(vectorlink::Joystick::Normalize(4095, reversed) == -1000, "joystick reverse");
}

void TestButtons()
{
  vectorlink::Buttons buttons;
  Expect(buttons.Update(1).current == 0, "button debounce sample one");
  Expect(buttons.Update(0).current == 0, "button bounce rejection");
  buttons.Update(1);
  buttons.Update(1);
  const auto pressed = buttons.Update(1);
  Expect(pressed.current == 1 && pressed.pressed == 1, "button pressed edge");
  const auto held = buttons.Update(1);
  Expect(held.pressed == 0 && held.current == 1, "button hold");
  buttons.Update(0);
  buttons.Update(0);
  const auto released = buttons.Update(0);
  Expect(released.current == 0 && released.released == 1, "button released edge");
}

void TestBattery()
{
  vectorlink::BatteryMonitor battery;
  Expect(vectorlink::BatteryMonitor::ConvertToMillivolts(2048) == 3301, "battery conversion");
  Expect(battery.Update(2606).percentage >= 99, "battery full percentage");
  Expect(battery.Update(2047).level == vectorlink::BatteryLevel::Critical, "battery critical");
  Expect(battery.Update(2048).level == vectorlink::BatteryLevel::Low, "battery low boundary");
}

void TestProtocol()
{
  vectorlink::protocol::ControlData control = {};
  control.sequence = 0x1234;
  control.axes = {-1000, -1, 1, 1000};
  control.buttons = 0x0A55;
  control.battery_mv = 3875;
  const auto packet = vectorlink::protocol::EncodeControl(control);
  Expect(packet[0] == 'V' && packet[1] == 'L', "protocol magic");
  Expect(packet[4] == 0x34 && packet[5] == 0x12, "protocol little endian");

  vectorlink::TelemetryState source = {};
  source.acknowledged_sequence = 42;
  source.receiver_battery_mv = 12000;
  source.values = {-20, 30, -400, 500};
  source.flags = 3;
  auto telemetry_packet = vectorlink::protocol::EncodeTelemetry(source, 9);
  vectorlink::TelemetryState decoded = {};
  Expect(vectorlink::protocol::DecodeTelemetry(telemetry_packet, decoded), "telemetry decode");
  Expect(decoded.values == source.values && decoded.flags == source.flags, "telemetry fields");
  telemetry_packet[10] ^= 1;
  Expect(!vectorlink::protocol::DecodeTelemetry(telemetry_packet, decoded), "protocol CRC reject");
}

void TestLinkQuality()
{
  vectorlink::LinkQuality quality;
  for (uint32_t index = 0; index < 32; ++index)
  {
    quality.Record((index % 2U) == 0U, index * 20U);
  }
  Expect(quality.SuccessRate() == 50, "link rolling success rate");
  Expect(quality.IsConnected(700, 500), "link connected");
  Expect(!quality.IsConnected(1200, 500), "link timeout");
}

} // namespace

int main()
{
  TestJoystick();
  TestButtons();
  TestBattery();
  TestProtocol();
  TestLinkQuality();
  std::cout << "All VectorLink host tests passed\n";
  return 0;
}
