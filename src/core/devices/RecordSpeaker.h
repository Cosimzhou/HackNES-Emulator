#pragma once

#include <fstream>
#include "../PeripheralDevices.h"

namespace hn {

class RecordSpeaker : public VirtualSpeaker {
 public:
  RecordSpeaker(const std::string& filepath, unsigned int channel = 1,
                unsigned int sample_rate = 44100);
  virtual ~RecordSpeaker();

  virtual void PushSample(std::int16_t* data, size_t count);

  virtual void Play() override;
  virtual void Stop() override;

  void SetOutSpeaker(VirtualSpeaker* speaker);

 protected:
  VirtualSpeaker* speaker_;

 private:
  DWord data_size_;
  std::string file_path_;
  std::fstream wav_file_;
};

}  // namespace hn
