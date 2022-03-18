#include "EmulatorSfml.h"

#include <fstream>

#include "PatternViewer.h"
#include "devices/RecordSpeaker.h"
#include "devices/VirtualJoypad.h"
#include "devices/VirtualScreen.h"
#include "devices/VirtualSpeaker.h"
#include "utils.h"

namespace hn {

EmulatorSfml::EmulatorSfml() {
  emulatorScreen_.reset(new VirtualScreenSfml);
  emulatorSpeaker_.reset(new RecordSpeaker("/tmp/xxx.wav"));
  dynamic_cast<RecordSpeaker*>(emulatorSpeaker_.get())
      ->SetOutSpeaker(new VirtualSpeakerSfml);
  emulatorJoypads_[0].reset(new VirtualJoypadSfml);
  emulatorJoypads_[1].reset(new VirtualJoypadSfml);
}

void EmulatorSfml::FrameRefresh() {
  window_.draw(dynamic_cast<VirtualScreenSfml&>(*emulatorScreen_));
  window_.display();
}

void EmulatorSfml::run() {
  if (!HardwareSetup()) {
    return;
  }

  window_.create(sf::VideoMode(NESVideoWidth * screenScale_,
                               NESVideoHeight * screenScale_),
                 "Hack NES", sf::Style::Titlebar | sf::Style::Close);
  window_.setVerticalSyncEnabled(true);
  emulatorScreen_->create(NESVideoWidth, NESVideoHeight, screenScale_, 0x30);

  Reset();
  RestoreRecord();

  sf::Event event;
  bool focus = true, pause = false;
  while (window_.isOpen()) {
    while (window_.pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed &&
           event.key.code == sf::Keyboard::Escape)) {
        window_.close();
        return;
      } else if (event.type == sf::Event::GainedFocus) {
        focus = true;
        cycleTimer_ = std::chrono::high_resolution_clock::now();
        OnResume();
      } else if (event.type == sf::Event::LostFocus) {
        focus = false;
        OnPause();
      } else if (event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F12) {
        CaptureImage();
        HintText("Screen captured");
      } else if (event.type == sf::Event::KeyPressed &&
                 event.key.code == sf::Keyboard::F1) {
        Reset();
        HintText("Game reset");
      } else if (event.type == sf::Event::KeyPressed &&
                 event.key.code == sf::Keyboard::F2) {
        pause = !pause;
        if (pause) {
          OnPause();
          HintText("Game is pausing");
        } else {
          OnResume();
          HintText("Game resumes");
          cycleTimer_ = std::chrono::high_resolution_clock::now();
        }
      } else if (pause && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F3) {
        // for (int i = 0; i < 29781; ++i) {  // Around one frame
        //  XPUTick();
        //}
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F3) {
        DebugDump();
        HintText("Info dumped");
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F4) {
        SaveRecord();
        HintText("Save recording");
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F5) {
        ToggleWorkMode();
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F6) {
        OnPause();

        PatternViewer pv;
        pv.setCartridge(cartridge_);
        pv.run();

        OnResume();
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F7) {
        goldfinger_.Toggle();
        HintText(goldfinger_.IsWorking() ? "Gold finger enable"
                                         : "Gold finger disable");
      }
    }

    RunTick(focus && !pause);
  }
}

void EmulatorSfml::CaptureImage() {
  sf::Texture texture;
  texture.create(window_.getSize().x, window_.getSize().y);
  texture.update(window_);

  sf::Image image = texture.copyToImage();
  image.saveToFile(Helper::GenImageCaptureName());
}

}  // namespace hn
