#include "buzzer.hpp"

extern "C"
{
#include "tim.h"
}

namespace vectorlink
{
namespace
{

constexpr Buzzer::Segment kKeyPress[] = {{2600, 35}};
constexpr Buzzer::Segment kStartup[] = {{1800, 70}, {0, 40}, {2600, 90}};
constexpr Buzzer::Segment kLowBattery[] = {{1500, 120}, {0, 100}, {1500, 120}};
constexpr Buzzer::Segment kLinkLost[] = {{900, 180}, {0, 120}, {900, 180}};
constexpr Buzzer::Segment kRadioError[] = {{700, 250}, {0, 100}, {700, 250}, {0, 100}, {700, 250}};

} // namespace

bool Buzzer::Initialize()
{
  Stop();
  return true;
}

void Buzzer::Play(BuzzerPattern pattern)
{
  if (pattern == BuzzerPattern::None || Priority(pattern) < Priority(pattern_))
  {
    return;
  }

  // Pattern tables contain tone and silence segments. Advancing them in Update() keeps Play()
  // non-blocking for all callers.
  pattern_ = pattern;
  segment_index_ = 0;
  const Segment* segments = Segments(pattern_, segment_count_);
  if (segments == nullptr || segment_count_ == 0)
  {
    Stop();
    return;
  }
  StartSegment(segments[0]);
}

void Buzzer::Update(uint32_t elapsed_ms)
{
  if (pattern_ == BuzzerPattern::None)
  {
    return;
  }

  if (elapsed_ms < remaining_ms_)
  {
    remaining_ms_ -= elapsed_ms;
    return;
  }

  uint8_t count = 0;
  const Segment* segments = Segments(pattern_, count);
  ++segment_index_;
  if (segments == nullptr || segment_index_ >= count)
  {
    Stop();
    return;
  }
  StartSegment(segments[segment_index_]);
}

void Buzzer::Stop()
{
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
  pattern_ = BuzzerPattern::None;
  segment_index_ = 0;
  segment_count_ = 0;
  remaining_ms_ = 0;
}

uint8_t Buzzer::Priority(BuzzerPattern pattern)
{
  return static_cast<uint8_t>(pattern);
}

const Buzzer::Segment* Buzzer::Segments(BuzzerPattern pattern, uint8_t& count)
{
  switch (pattern)
  {
    case BuzzerPattern::KeyPress:
      count = sizeof(kKeyPress) / sizeof(kKeyPress[0]);
      return kKeyPress;
    case BuzzerPattern::Startup:
      count = sizeof(kStartup) / sizeof(kStartup[0]);
      return kStartup;
    case BuzzerPattern::LowBattery:
      count = sizeof(kLowBattery) / sizeof(kLowBattery[0]);
      return kLowBattery;
    case BuzzerPattern::LinkLost:
      count = sizeof(kLinkLost) / sizeof(kLinkLost[0]);
      return kLinkLost;
    case BuzzerPattern::RadioError:
      count = sizeof(kRadioError) / sizeof(kRadioError[0]);
      return kRadioError;
    default:
      count = 0;
      return nullptr;
  }
}

void Buzzer::StartSegment(const Segment& segment)
{
  remaining_ms_ = segment.duration_ms;
  if (segment.frequency_hz == 0)
  {
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    return;
  }

  // TIM4 runs at a 1 MHz counter rate, so the period is expressed directly in microseconds.
  const uint32_t period = 1000000U / segment.frequency_hz;
  __HAL_TIM_SET_AUTORELOAD(&htim4, period - 1U);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, period / 2U);
  __HAL_TIM_SET_COUNTER(&htim4, 0);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
}

} // namespace vectorlink
