#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

#include "PeripheralDevices.h"
#include "glog/logging.h"

namespace hn {

inline std::string trim(const std::string &s) {
  if (s.empty()) {
    return s;
  }

  const char *space = " \f\r\v\n\t";
  std::string str(s);
  str.erase(0, str.find_first_not_of(space));
  str.erase(str.find_last_not_of(space) + 1);
  return str;
}

const std::string kButtonStrings[] = {"A",  "B",    "SELECT", "START",
                                      "UP", "DOWN", "LEFT",   "RIGHT"};

const std::string kKeyStrings[] = {
    "A",         "B",        "C",        "D",        "E",        "F",
    "G",         "H",        "I",        "J",        "K",        "L",
    "M",         "N",        "O",        "P",        "Q",        "R",
    "S",         "T",        "U",        "V",        "W",        "X",
    "Y",         "Z",        "NUM0",     "NUM1",     "NUM2",     "NUM3",
    "NUM4",      "NUM5",     "NUM6",     "NUM7",     "NUM8",     "NUM9",
    "ESCAPE",    "LCONTROL", "LSHIFT",   "LALT",     "LSYSTEM",  "RCONTROL",
    "RSHIFT",    "RALT",     "RSYSTEM",  "MENU",     "LBRACKET", "RBRACKET",
    "SEMICOLON", "COMMA",    "PERIOD",   "QUOTE",    "SLASH",    "BACKSLASH",
    "TILDE",     "EQUAL",    "DASH",     "SPACE",    "RETURN",   "BACKSPACE",
    "TAB",       "PAGEUP",   "PAGEDOWN", "END",      "HOME",     "INSERT",
    "DELETE",    "ADD",      "SUBTRACT", "MULTIPLY", "DIVIDE",   "LEFT",
    "RIGHT",     "UP",       "DOWN",     "NUMPAD0",  "NUMPAD1",  "NUMPAD2",
    "NUMPAD3",   "NUMPAD4",  "NUMPAD5",  "NUMPAD6",  "NUMPAD7",  "NUMPAD8",
    "NUMPAD9",   "F1",       "F2",       "F3",       "F4",       "F5",
    "F6",        "F7",       "F8",       "F9",       "F10",      "F11",
    "F12",       "F13",      "F14",      "F15",      "PAUSE"};

int parseKeyName(const std::string &name) {
  auto beg = std::begin(kButtonStrings);
  auto it = std::find(beg, std::end(kButtonStrings), trim(name));

  return std::distance(beg, it);
}

void parseControllerConf(std::string filepath, JoypadInputConfig &p1,
                         JoypadInputConfig &p2) {
  std::ifstream file(filepath);
  std::string line;
  enum { Player1, Player2, None } state;
  int line_no = 0;
  while (std::getline(file, line)) {
    line = trim(line);
    if (line[0] == '#' || line.empty()) {
      continue;
    }

    std::transform(line.begin(), line.end(), line.begin(), toupper);
    if (line == "[PLAYER1]") {
      state = Player1;
    } else if (line == "[PLAYER2]") {
      state = Player2;
    } else if (state == Player1 || state == Player2) {
      auto divider = line.find("=");
      if (line.size() >= 5 && line.substr(0, 3) == "JOY") {
        auto &joy = (state == Player1 ? p1 : p2).joystick_;
        int keyIdx = atol(trim(line.substr(divider + 1)).c_str());

        if (line.at(3) != '_') {
          joy.index = keyIdx;
          joy.inUse = true;
          VLOG(3) << "Use joy stick " << keyIdx << " for " << state;
        } else if (line.substr(0, 4) == "JOY_") {
          auto i = parseKeyName(line.substr(4, divider - 4));
          if (i >= joy.keyBindings_.size()) {
            LOG(ERROR) << "Invalid key in configuration file at Line "
                       << line_no;
            continue;
          }

          joy.keyBindings_[i] = keyIdx;
          VLOG(3) << "Use joy stick key:" << keyIdx << " for " << state
                  << " key:" << i;
        } else {
          LOG(ERROR) << "Invalid line in key configuration at Line " << line_no;
        }
      } else {
        auto i = parseKeyName(line.substr(0, divider));
        auto it2 = std::find(std::begin(kKeyStrings), std::end(kKeyStrings),
                             trim(line.substr(divider + 1)));
        if (i >= 8 || it2 == std::end(kKeyStrings)) {
          LOG(ERROR) << "Invalid key in configuration file at Line " << line_no;
          continue;
        }

        auto key = std::distance(std::begin(kKeyStrings), it2);
        (state == Player1 ? p1 : p2).keyboard_[i] = static_cast<int>(key);

        VLOG(3) << "Player" << state << " key" << kButtonStrings[i] << "=> "
                << kKeyStrings[key];
      }
    } else {
      LOG(ERROR) << "Invalid line in key configuration at Line " << line_no;
    }

    ++line_no;
  }
}
}  // namespace hn
