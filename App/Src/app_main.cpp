#include "app_main.h"

#include "app_config.hpp"
#include "app_state.hpp"
#include "battery_monitor.hpp"
#include "board_adc.hpp"
#include "board_gpio.hpp"
#include "buttons.hpp"
#include "buzzer.hpp"
#include "control_protocol.hpp"
#include "joystick.hpp"
#include "link_quality.hpp"
#include "nrf24l01.hpp"
#include "oled_display.hpp"

extern "C"
{
#include "FreeRTOS.h"
#include "main.h"
#include "task.h"
}

#include <array>

namespace vectorlink
{
namespace
{

constexpr uint16_t kInputTaskStackDepth = 256;
constexpr uint16_t kRadioTaskStackDepth = 384;
constexpr uint16_t kUiTaskStackDepth = 384;
constexpr UBaseType_t kInputTaskPriority = tskIDLE_PRIORITY + 3;
constexpr UBaseType_t kRadioTaskPriority = tskIDLE_PRIORITY + 4;
constexpr UBaseType_t kUiTaskPriority = tskIDLE_PRIORITY + 2;
constexpr uint32_t kRadioTransmitTimeoutMs = 8;
constexpr uint32_t kAlarmRepeatMs = 10000;

AppSnapshot g_state = {};
BuzzerPattern g_pending_buzzer = BuzzerPattern::None;
TaskHandle_t g_radio_task_handle = nullptr;
Buzzer g_buzzer;
OledDisplay g_oled;

const std::array<JoystickAxisConfig, 4> kJoystickConfig = {{
    {config::kJoystickMinimum, config::kJoystickCenter, config::kJoystickMaximum,
     config::kJoystickDeadZone, config::kJoystickReversed[0]},
    {config::kJoystickMinimum, config::kJoystickCenter, config::kJoystickMaximum,
     config::kJoystickDeadZone, config::kJoystickReversed[1]},
    {config::kJoystickMinimum, config::kJoystickCenter, config::kJoystickMaximum,
     config::kJoystickDeadZone, config::kJoystickReversed[2]},
    {config::kJoystickMinimum, config::kJoystickCenter, config::kJoystickMaximum,
     config::kJoystickDeadZone, config::kJoystickReversed[3]},
}};

AppSnapshot ReadState()
{
  // Copy the small aggregate atomically so consumers see fields from one logical update without
  // introducing a queue or mutex for this single-board application.
  taskENTER_CRITICAL();
  const AppSnapshot state = g_state;
  taskEXIT_CRITICAL();
  return state;
}

void WriteInputState(const InputState& input)
{
  taskENTER_CRITICAL();
  g_state.input = input;
  taskEXIT_CRITICAL();
}

void WriteRadioState(const RadioState& radio, const TelemetryState& telemetry)
{
  taskENTER_CRITICAL();
  g_state.radio = radio;
  g_state.telemetry = telemetry;
  taskEXIT_CRITICAL();
}

void SetOledReady(bool ready)
{
  taskENTER_CRITICAL();
  g_state.oled_ready = ready;
  taskEXIT_CRITICAL();
}

void TogglePage()
{
  taskENTER_CRITICAL();
  g_state.page ^= 1U;
  taskEXIT_CRITICAL();
}

void RequestBuzzer(BuzzerPattern pattern)
{
  // Enum values encode priority, preventing a key click from replacing a pending fault indication.
  taskENTER_CRITICAL();
  if (static_cast<uint8_t>(pattern) > static_cast<uint8_t>(g_pending_buzzer))
  {
    g_pending_buzzer = pattern;
  }
  taskEXIT_CRITICAL();
}

BuzzerPattern TakeBuzzerRequest()
{
  taskENTER_CRITICAL();
  const BuzzerPattern pattern = g_pending_buzzer;
  g_pending_buzzer = BuzzerPattern::None;
  taskEXIT_CRITICAL();
  return pattern;
}

void InputTaskEntry(void*)
{
  // Task-local objects retain filter and debounce history for the lifetime of the task.
  Joystick joystick(kJoystickConfig);
  Buttons buttons;
  BatteryMonitor battery;
  TickType_t wake_time = xTaskGetTickCount();

  for (;;)
  {
    const BoardAdc::Snapshot adc = BoardAdc::ReadSnapshot();
    const std::array<uint16_t, 4> raw_axes = {adc[0], adc[1], adc[2], adc[3]};
    const ButtonState button_state = buttons.Update(BoardGpio::ReadPressedButtons());

    InputState input = {};
    input.axes = joystick.Process(raw_axes);
    input.buttons = button_state.current;
    input.pressed = button_state.pressed;
    input.released = button_state.released;
    input.battery = battery.Update(adc[4]);
    WriteInputState(input);

    constexpr uint16_t kKey9Mask = 1U << 8U;
    if ((button_state.pressed & kKey9Mask) != 0U)
    {
      TogglePage();
      RequestBuzzer(BuzzerPattern::KeyPress);
    }

    // DelayUntil keeps a stable sampling phase instead of accumulating execution-time jitter.
    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(config::kInputPeriodMs));
  }
}

void RadioTaskEntry(void*)
{
  Nrf24l01 radio;
  LinkQuality link_quality;
  RadioState radio_state = {};
  TelemetryState telemetry = {};
  TickType_t wake_time = xTaskGetTickCount();
  uint32_t last_initialization_attempt_ms = 0;
  bool link_was_connected = false;

  for (;;)
  {
    const uint32_t now_ms = HAL_GetTick();
    if (!radio_state.initialized)
    {
      // Throttled retries keep a missing radio from consuming CPU or starving other tasks.
      if (now_ms - last_initialization_attempt_ms >= 1000U || last_initialization_attempt_ms == 0U)
      {
        last_initialization_attempt_ms = now_ms;
        radio_state.initialized = radio.Initialize({config::kRadioChannel, config::kRadioAddress});
        if (!radio_state.initialized)
        {
          RequestBuzzer(BuzzerPattern::RadioError);
        }
      }
      WriteRadioState(radio_state, telemetry);
      vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(config::kRadioPeriodMs));
      continue;
    }

    const AppSnapshot snapshot = ReadState();
    protocol::ControlData control = {};
    control.sequence = ++radio_state.sequence;
    control.axes = snapshot.input.axes;
    control.buttons = snapshot.input.buttons;
    control.battery_mv = snapshot.input.battery.voltage_mv;
    control.flags = snapshot.input.battery.level != BatteryLevel::Normal ? 1U : 0U;
    control.uptime_seconds = static_cast<uint16_t>(now_ms / 1000U);
    const protocol::Packet packet = protocol::EncodeControl(control);

    bool transmitted = false;
    RadioTransmitResult result = RadioTransmitResult::BusError;
    uint8_t received_length = 0;
    std::array<uint8_t, Nrf24l01::kMaximumPayloadSize> received = {};
    ++radio_state.transmitted;
    if (radio.StartTransmit(packet.data(), packet.size()))
    {
      // EXTI gives the notification. The deadline also covers a missing IRQ or receiver ACK.
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kRadioTransmitTimeoutMs));
      result = radio.CompleteTransmit(received.data(), received_length);
      if (result == RadioTransmitResult::Pending)
      {
        result = radio.CheckTransmitTimeout(HAL_GetTick(), kRadioTransmitTimeoutMs);
      }
      transmitted = result == RadioTransmitResult::Success;
    }

    if (result == RadioTransmitResult::BusError)
    {
      radio_state.initialized = false;
      RequestBuzzer(BuzzerPattern::RadioError);
    }

    link_quality.Record(transmitted, HAL_GetTick());
    if (transmitted)
    {
      ++radio_state.succeeded;
      if (received_length == protocol::kPacketSize)
      {
        // The driver owns a 32-byte buffer; the protocol accepts only its exact 24-byte packet.
        protocol::Packet telemetry_packet = {};
        for (size_t index = 0; index < telemetry_packet.size(); ++index)
        {
          telemetry_packet[index] = received[index];
        }
        if (protocol::DecodeTelemetry(telemetry_packet, telemetry))
        {
          ++radio_state.received;
        }
      }
    }
    else
    {
      ++radio_state.failed;
    }

    radio_state.success_rate = link_quality.SuccessRate();
    radio_state.connected = link_quality.IsConnected(HAL_GetTick(), config::kLinkLostTimeoutMs);
    if (link_was_connected && !radio_state.connected)
    {
      RequestBuzzer(BuzzerPattern::LinkLost);
    }
    link_was_connected = radio_state.connected;
    WriteRadioState(radio_state, telemetry);
    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(config::kRadioPeriodMs));
  }
}

void UiTaskEntry(void*)
{
  // UI is the sole owner of I2C display traffic and PWM sound sequencing, so no bus mutex is
  // needed.
  g_buzzer.Initialize();
  g_buzzer.Play(BuzzerPattern::Startup);
  SetOledReady(g_oled.Initialize());

  TickType_t wake_time = xTaskGetTickCount();
  uint32_t display_elapsed_ms = 0;
  uint32_t last_oled_attempt_ms = 0;
  uint32_t last_battery_alarm_ms = 0;
  uint32_t last_link_alarm_ms = 0;

  for (;;)
  {
    const uint32_t now_ms = HAL_GetTick();
    g_buzzer.Play(TakeBuzzerRequest());
    g_buzzer.Update(config::kUiUpdatePeriodMs);

    const AppSnapshot snapshot = ReadState();
    if (!snapshot.oled_ready && now_ms - last_oled_attempt_ms >= 5000U)
    {
      last_oled_attempt_ms = now_ms;
      SetOledReady(g_oled.Initialize());
    }
    if (snapshot.input.battery.level != BatteryLevel::Normal &&
        now_ms - last_battery_alarm_ms >= kAlarmRepeatMs)
    {
      last_battery_alarm_ms = now_ms;
      g_buzzer.Play(BuzzerPattern::LowBattery);
    }
    if (snapshot.radio.initialized && !snapshot.radio.connected && snapshot.radio.transmitted > 0 &&
        now_ms - last_link_alarm_ms >= kAlarmRepeatMs)
    {
      last_link_alarm_ms = now_ms;
      g_buzzer.Play(BuzzerPattern::LinkLost);
    }

    display_elapsed_ms += config::kUiUpdatePeriodMs;
    if (display_elapsed_ms >= config::kDisplayPeriodMs)
    {
      display_elapsed_ms = 0;
      g_oled.Render(snapshot);
      if (!g_oled.IsReady())
      {
        SetOledReady(false);
      }
    }

    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(config::kUiUpdatePeriodMs));
  }
}

} // namespace

class Application final
{
public:
  static void Initialize()
  {
    // ADC DMA must be running before InputTask starts reading channel snapshots.
    if (!BoardAdc::Initialize())
    {
      Error_Handler();
    }

    if (xTaskCreate(InputTaskEntry, "Input", kInputTaskStackDepth, nullptr, kInputTaskPriority,
                    nullptr) != pdPASS ||
        xTaskCreate(RadioTaskEntry, "Radio", kRadioTaskStackDepth, nullptr, kRadioTaskPriority,
                    &g_radio_task_handle) != pdPASS ||
        xTaskCreate(UiTaskEntry, "UI", kUiTaskStackDepth, nullptr, kUiTaskPriority, nullptr) !=
            pdPASS)
    {
      Error_Handler();
    }
  }
};

} // namespace vectorlink

extern "C" void VectorLink_InitializeApp(void)
{
  vectorlink::Application::Initialize();
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
  if (gpio_pin != NRF_IRQ_Pin || vectorlink::g_radio_task_handle == nullptr)
  {
    return;
  }

  BaseType_t higher_priority_task_woken = pdFALSE;
  // Never touch SPI in EXTI context; wake the task that exclusively owns the radio bus.
  vTaskNotifyGiveFromISR(vectorlink::g_radio_task_handle, &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}
