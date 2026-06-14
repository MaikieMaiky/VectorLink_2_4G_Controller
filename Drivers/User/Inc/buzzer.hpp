#ifndef VECTORLINK_BUZZER_HPP
#define VECTORLINK_BUZZER_HPP

#include <cstdint>

namespace vectorlink
{

/** @brief Named sound patterns ordered from lowest to highest interruption priority. */
enum class BuzzerPattern : uint8_t
{
  None,
  KeyPress,
  Startup,
  LowBattery,
  LinkLost,
  RadioError,
};

/** @brief Non-blocking TIM4 PWM sound sequencer owned by the UI task. */
class Buzzer final
{
public:
  struct Segment
  {
    uint16_t frequency_hz;
    uint16_t duration_ms;
  };

  /** Stops PWM and resets the sequencer. */
  bool Initialize();

  /** Starts a pattern unless a higher-priority pattern is already active. */
  void Play(BuzzerPattern pattern);

  /** Advances the active pattern by the elapsed scheduler time. */
  void Update(uint32_t elapsed_ms);

  /** Immediately disables PWM and clears the active pattern. */
  void Stop();

private:
  static uint8_t Priority(BuzzerPattern pattern);
  static const Segment* Segments(BuzzerPattern pattern, uint8_t& count);
  void StartSegment(const Segment& segment);

  BuzzerPattern pattern_ = BuzzerPattern::None;
  uint8_t segment_index_ = 0;
  uint8_t segment_count_ = 0;
  uint32_t remaining_ms_ = 0;
};

} // namespace vectorlink

#endif // VECTORLINK_BUZZER_HPP
