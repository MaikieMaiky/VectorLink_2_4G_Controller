#ifndef VECTORLINK_BUZZER_HPP
#define VECTORLINK_BUZZER_HPP

#include <cstdint>

namespace vectorlink
{

enum class BuzzerPattern : uint8_t
{
  None,
  KeyPress,
  Startup,
  LowBattery,
  LinkLost,
  RadioError,
};

class Buzzer final
{
public:
  struct Segment
  {
    uint16_t frequency_hz;
    uint16_t duration_ms;
  };

  bool Initialize();
  void Play(BuzzerPattern pattern);
  void Update(uint32_t elapsed_ms);
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
