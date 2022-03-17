#pragma once

#include <SFML/Audio.hpp>
#include "../PeripheralDevices.h"

namespace hn {

class VirtualSpeakerSfml : public VirtualSpeaker, public sf::SoundStream {
 public:
  VirtualSpeakerSfml(unsigned int channel = 1,
                     unsigned int sample_rate = 44100);
  virtual ~VirtualSpeakerSfml() = default;

  virtual void PushSample(std::int16_t *data, size_t count);

  virtual void Play();
  virtual void Stop();

 protected:
  virtual bool onGetData(sf::SoundStream::Chunk &data);
  virtual void onSeek(sf::Time timeOffset);

 private:
  size_t queue_index_;
  std::vector<int16_t> mute_sample_;
  std::vector<std::vector<int16_t>> queue_buffer_;

  sf::SoundStream::Chunk chunk_;
};

}  // namespace hn
