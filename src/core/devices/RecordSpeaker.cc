#include "RecordSpeaker.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>

#include "glog/logging.h"

namespace hn {

// WAVEFILE Header Format
//
// 1 - 4	“RIFF”	Marks the file as a riff file. Characters are each 1
//        byte long.
// 5 - 8	File size (integer)	Size of the overall file - 8 bytes, in
//        bytes (32-bit integer). Typically, you’d fill this in after creation.
// 9 -12  “WAVE” File Type Header. For our purposes, it always “WAVE”.
// 13-16	“fmt "	Format chunk marker. Includes trailing null
// 17-20	16	Length of format data as listed above
// 21-22	1	Type of format (1 is PCM) - 2 byte integer
// 23-24	2	Number of Channels - 2 byte integer
// 25-28	44100	Sample Rate - 32 byte integer. Common values are 44100
//        (CD), 48000 (DAT). Sample Rate = Number of Samples per second, or
//        Hertz.
// 29-32  176400	(Sample Rate * BitsPerSample * Channels) / 8.
// 33-34	4 (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit
//        stereo/16 bit  mono4 - 16 bit stereo
// 35-36	16	Bits per sample
// 37-40	“data” “data” chunk header. Marks the beginning of the data
//         section.
// 41-44  File size (data)	Size of the data section.
//
const Byte kWaveFileHeader[] = {
    0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00,  // "RIFF" & file size
    0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,  // "WAVEfmt "
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,  // 16 & PCM Mode
    0x44, 0xac, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00,  // 44100 & 44100*2
    0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61,  // 2 bytes, 16, "data"
    0x00, 0x00, 0x00, 0x00                           // Chunk size
};

RecordSpeaker::RecordSpeaker(const std::string &filepath, unsigned int channel,
                             unsigned int sample_rate)
    : VirtualSpeaker(channel, sample_rate),
      data_size_(0),
      file_path_(filepath),
      wav_file_(filepath, std::ios::in | std::ios::out | std::ios::trunc |
                              std::ios::binary) {
  // Write wave file header
  wav_file_.write(reinterpret_cast<const char *>(&kWaveFileHeader[0]),
                  sizeof(kWaveFileHeader));
}

RecordSpeaker::~RecordSpeaker() {
  // Write data chunk size
  wav_file_.seekp(40, std::ios_base::beg);
  wav_file_.write(reinterpret_cast<const char *>(&data_size_), 4);

  // Write file size
  data_size_ += 36;
  wav_file_.seekp(4, std::ios_base::beg);
  wav_file_.write(reinterpret_cast<const char *>(&data_size_), 4);

  wav_file_.close();
}

void RecordSpeaker::PushSample(std::int16_t *data, size_t count) {
  if (speaker_) speaker_->PushSample(data, count);

  data_size_ += count;
  wav_file_.write(reinterpret_cast<const char *>(data),
                  sizeof(std::int16_t) * count);
}

void RecordSpeaker::SetOutSpeaker(VirtualSpeaker *speaker) {
  speaker_.reset(speaker);
}

void RecordSpeaker::Play() {
  if (speaker_) speaker_->Play();
}

void RecordSpeaker::Stop() {
  if (speaker_) speaker_->Stop();
}

}  // namespace hn
