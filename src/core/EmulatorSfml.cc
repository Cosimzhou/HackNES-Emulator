#include "EmulatorSfml.h"

#include <fstream>

#include "PatternViewer.h"
#include "devices/RecordScreen.h"
#include "devices/RecordSpeaker.h"
#include "devices/SfmlJoypad.h"
#include "devices/SfmlScreen.h"
#include "devices/SfmlSpeaker.h"
#include "utils.h"

namespace hn {

EmulatorSfml::EmulatorSfml() : Emulator(), sfScreen_(new VirtualScreenSfml) {
  if (true) {
    emulatorScreen_.reset(new RecordScreen(Helper::SequenceImageName()));
    dynamic_cast<RecordScreen*>(emulatorScreen_.get())->SetOutScreen(sfScreen_);

    emulatorSpeaker_.reset(new RecordSpeaker(Helper::GenSoundRecordName()));
    dynamic_cast<RecordSpeaker*>(emulatorSpeaker_.get())
        ->SetOutSpeaker(new VirtualSpeakerSfml);
  } else {
    emulatorScreen_.reset(sfScreen_);
    emulatorSpeaker_.reset(new VirtualSpeakerSfml);
  }

  emulatorJoypads_[0].reset(new VirtualJoypadSfml);
  emulatorJoypads_[1].reset(new VirtualJoypadSfml);
}

void EmulatorSfml::FrameRefresh() {
  window_.draw(*sfScreen_);
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

  pausing_ = false;

  Reset();
  RestoreRecord();

  sf::Event event;
  bool focus = true;
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
        GetFocus();
      } else if (event.type == sf::Event::LostFocus) {
        focus = false;
        LostFocus();
      } else if (event.type == sf::Event::KeyPressed &&
                 event.key.code == sf::Keyboard::F1) {
        Reset();
        HintText("Game reset");
      } else if (focus && event.type == sf::Event::KeyReleased) {
        switch (event.key.code) {
          case sf::Keyboard::F2:
            OnPauseToggle();
            break;
          case sf::Keyboard::F3:
            OnDebug();
            break;
          case sf::Keyboard::F4:
            OnSaveRecord();
            break;
          case sf::Keyboard::F5:
            ToggleWorkMode();
            break;
          case sf::Keyboard::F6:
            OnPatternView();
            break;
          case sf::Keyboard::F7:
            OnGoldFingerToggle();
            break;
          case sf::Keyboard::F12:
            CaptureImage();
            HintText("Screen captured");
            break;
        }
      }
    }

    // Add some emulater shortcut on joypad
    if (true /**/ && focus) {
      static bool jpAction = true;
      if (sf::Joystick::isButtonPressed(0, 6)) {
        if (jpAction) OnGoldFingerToggle();
        jpAction = false;
      } else if (sf::Joystick::isButtonPressed(0, 4)) {
        if (jpAction) OnSaveRecord();
        jpAction = false;
      } else if (sf::Joystick::isButtonPressed(0, 5)) {
        if (jpAction) OnPauseToggle();
        jpAction = false;
      } else {
        jpAction = true;
      }
    }

    RunTick(focus && !pausing_);
  }
}

void EmulatorSfml::CaptureImage() {
  sf::Texture texture;
  texture.create(window_.getSize().x, window_.getSize().y);
  texture.update(window_);

  sf::Image image = texture.copyToImage();
  image.saveToFile(Helper::GenImageCaptureName());
}

void EmulatorSfml::OnGoldFingerToggle() {
  goldfinger_.Toggle();
  HintText(goldfinger_.IsWorking() ? "Gold finger enable"
                                   : "Gold finger disable");
}
void EmulatorSfml::OnSaveRecord() {
  SaveRecord();
  HintText("Save recording");
}

void EmulatorSfml::OnPatternView() {
  LostFocus();

  PatternViewer pv;
  pv.setVideoScale(screenScale_);
  if (cartridge_.getVROM().empty())
    pv.setRom(&mapper_->VRAM());
  else
    pv.setCartridge(cartridge_);
  pv.run();

  GetFocus();
}

void EmulatorSfml::OnDebug() {
  DebugDump();
  HintText("Info dumped");
}

void EmulatorSfml::OnPauseToggle() {
  pausing_ = !pausing_;
  if (pausing_) {
    LostFocus();
    HintText("Game is pausing");
    FrameRefresh();
  } else {
    GetFocus();
    HintText("Game resumes");
    cycleTimer_ = std::chrono::high_resolution_clock::now();
  }
}

void EmulatorSfml::OnPause() {
  pausing_ = false;
  OnPauseToggle();
}

}  // namespace hn
