#include "SfmlSpeaker.h"

#include <algorithm>
#include <iterator>
#include <limits>

#include "glog/logging.h"

namespace hn {

constexpr size_t kSampleLeastSize = 1024;
VirtualSpeakerSfml::VirtualSpeakerSfml(unsigned int channel,
                                       unsigned int sample_rate)
    : VirtualSpeaker(channel, sample_rate), queue_index_(0) {
  queue_buffer_.resize(40);
  mute_sample_.resize(kSampleLeastSize * 2, 0);
  chunk_.samples = mute_sample_.data();
  chunk_.sampleCount = kSampleLeastSize;

  initialize(channel, sample_rate);
  setLoop(true);
}

void VirtualSpeakerSfml::PushSample(std::int16_t *data, size_t count) {
  auto &current_buffer = queue_buffer_[queue_index_];
  if (current_buffer.size() >= kSampleLeastSize) {
    chunk_.samples = current_buffer.data();
    chunk_.sampleCount = current_buffer.size();
    if (++queue_index_ >= queue_buffer_.size()) {
      queue_index_ = 0;
    }

    queue_buffer_[queue_index_].clear();
  } else {
    std::copy(data, data + count, std::back_inserter(current_buffer));
  }
}

bool VirtualSpeakerSfml::onGetData(sf::SoundStream::Chunk &data) {
  data = chunk_;
  VLOG(12) << "get " << chunk_.samples[2] << " ~ " << chunk_.sampleCount;
  return true;
}

void VirtualSpeakerSfml::onSeek(sf::Time timeOffset) {
  VLOG(12) << timeOffset.asSeconds();
}

void VirtualSpeakerSfml::Play() { play(); }
void VirtualSpeakerSfml::Stop() { stop(); }
}  // namespace hn
