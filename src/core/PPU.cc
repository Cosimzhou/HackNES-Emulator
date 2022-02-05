#include "PPU.h"

#include <cstring>
#include <ios>

#include "CPU.h"
#include "common.h"
#include "glog/logging.h"

#ifdef PPUMASK_IN_BYTE
#define GREY_SCALE_MODE() (ppu_mask_ & 0x1)
#define SHOW_SPRITES() (ppu_mask_ & 0x10)
#define SHOW_EDGE_SPRITES() (ppu_mask_ & 0x4)
#define SHOW_BACKGROUND() (ppu_mask_ & 0x8)
#define SHOW_EDGE_BACKGROUND() (ppu_mask_ & 0x2)
#else  // PPUMASK_IN_BYTE
#define GREY_SCALE_MODE() greyscaleMode_
#define SHOW_SPRITES() showSprites_
#define SHOW_EDGE_SPRITES() showEdgeSprites_
#define SHOW_BACKGROUND() showBackground_
#define SHOW_EDGE_BACKGROUND() showEdgeBackground_
#endif  // PPUMASK_IN_BYTE

#ifdef PPUCONTROL_IN_BYTE
#define LONG_SPRITE() (ppu_control_ & 0x20)
#define GEN_INTERRUPT() (ppu_control_ & 0x80)
#define HIGH_BG_PAGE() (ppu_control_ & 0x10)
#define HIGH_SPR_PAGE() (ppu_control_ & 0x8)
#else  // PPUCONTROL_IN_BYTE
#define LONG_SPRITE() longSprites_
#define GEN_INTERRUPT() generateInterrupt_
#define HIGH_BG_PAGE() (bgPage_ == High)
#define HIGH_SPR_PAGE() (sprPage_ == High)
#endif  // PPUCONTROL_IN_BYTE

namespace hn {
constexpr int ScanlineCycleLength = 341;
constexpr int FrameEndScanline = 261;

constexpr int AttributeOffset = 0x3C0;

#ifdef PPUCONTROL_IN_BYTE
constexpr Byte kPPUCtrlAddrIncUnit = 0x4;
constexpr Byte kPPUCtrlSprPageHigh = 0x8;
constexpr Byte kPPUCtrlBgPageHigh = 0x10;
constexpr Byte kPPUCtrlLongSprite = 0x20;
constexpr Byte kPPUCtrlInterrupt = 0x80;
#endif  // PPUSTATUS_IN_BYTE

PPU::PPU(MainBus &mainBus, PictureBus &bus, VirtualScreen &screen)
    : mainBus_(mainBus),
      bus_(bus),
      screen_(screen),
      spriteMemory_(64 * 4),
      pictureBuffer_(ScanlineVisibleDots,
                     std::vector<Color>(VisibleScanlines, 0x24)) {}

void PPU::Reset() {
#ifdef PPUCONTROL_IN_BYTE
  ppu_control_ = 0;
#else   // PPUCONTROL_IN_BYTE
  longSprites_ = generateInterrupt_ = false;
  bgPage_ = sprPage_ = Low;
#endif  // PPUCONTROL_IN_BYTE

#ifdef PPUMASK_IN_BYTE
  ppu_mask_ = 0x1e;
#else   // PPUMASK_IN_BYTE
  greyscaleMode_ = false;
  showEdgeBackground_ = true;
  showEdgeSprites_ = true;
  showBackground_ = true;
  showSprites_ = true;
#endif  // PPUMASK_IN_BYTE

  evenFrame_ = firstWrite_ = true;

#ifdef PPUSTATUS_IN_BYTE
  ppu_status_ = 0;
#else   // PPUSTATUS_IN_BYTE
  vblank_ = false;
#endif  // PPUSTATUS_IN_BYTE
  dataAddress_ = cycle_ = scanline_ = oamDataAddress_ = fineXScroll_ =
      tempAddress_ = 0;
  frameIndex_ = 0;
  // baseNameTable_ = 0x2000;
  dataAddrIncrement_ = 1;
  pipelineState_ = PreRender;
  scanlineSprites_.reserve(8);
  scanlineSprites_.resize(0);
}

void PPU::Step() {
  switch (pipelineState_) {
    case PreRender:
      preRender();
      break;
    case Render:
      render();
      break;
    case PostRender:
      postRender();
      break;
    case VerticalBlank:
      vBlank();
      break;
    default:
      LOG(ERROR) << "Well, this shouldn't have happened.";
  }

  ++cycle_;
}

void PPU::vBlank() {
  if (cycle_ >= ScanlineEndCycle) {
    // If cycle_ is greater then 339, the scanline increases.
    ++scanline_;
    cycle_ = 0;
  }

  if (scanline_ >= FrameEndScanline) {
    // If scanline_ is greater than 260, go to the next frame loop.
    pipelineState_ = PreRender;
    scanline_ = 0;
    evenFrame_ = !evenFrame_;
    frameIndex_++;
  }
}

void PPU::preRender() {
  bool doubleShow = SHOW_BACKGROUND() && SHOW_SPRITES();
  if (cycle_ == 1) {
#ifdef PPUSTATUS_IN_BYTE
    CLR_BIT(ppu_status_, 0xc0);
#else   // PPUSTATUS_IN_BYTE
    vblank_ = sprZeroHit_ = false;
#endif  // PPUSTATUS_IN_BYTE
  } else if (doubleShow) {
    if (cycle_ == ScanlineVisibleDots + 2) {
      // If cycle equals 258,
      // Set bits related to horizontal position
      // Unset horizontal bits
      SWAP_BIT(dataAddress_, tempAddress_, 0x41f);
    } else if (cycle_ > 280 && cycle_ <= 304) {
      // Set vertical bits
      // Unset bits related to horizontal
      // Copy
      SWAP_BIT(dataAddress_, tempAddress_, 0x7be0);
    }
  }

  // if rendering is on, every other frame is one cycle shorter (340 or 339)
  if (cycle_ >= ScanlineEndCycle - (!evenFrame_ && doubleShow)) {
    pipelineState_ = Render;
    cycle_ = scanline_ = 0;
  }
}

void PPU::render() {
  if (cycle_ > 0 && cycle_ <= ScanlineVisibleDots) {
    // If cycle_ is between 0 and 256, it indicates rendering in scanline.
    renderInScanline();
  } else if (cycle_ == ScanlineVisibleDots + 1 && SHOW_BACKGROUND()) {
    // If cycle_ is 257
    if (TEST_BITS(dataAddress_, 0x7000)) {    // if fine Y < 7
      CLR_BIT(dataAddress_, 0x7000);          // fine Y = 0
      Address tileY = dataAddress_ & 0x03E0;  // let y = coarse Y
      if (tileY == 29 * 0x20) {               // if tileY == 0x3a0
        tileY = 0;                            // coarse Y = 0
        dataAddress_ ^= 0x0800;               // switch vertical nametable
      } else if (tileY == 31 * 0x20) {        // if tileY == 0x3e0
        tileY = 0;  // coarse Y = 0, nametable not switched
      } else {
        tileY += 0x20;  // increment coarse Y
      }

      // put coarse Y back into dataAddress_
      SWAP_BIT(dataAddress_, tileY, 0x3e0);
    } else {
      dataAddress_ += 0x1000;  // increment fine Y
    }
  } else if (cycle_ == ScanlineVisibleDots + 2 && SHOW_BACKGROUND() &&
             SHOW_SPRITES()) {
    // If cycle_ is 258 and double show, copy bits related to horizontal
    // position
    SWAP_BIT(dataAddress_, tempAddress_, 0x41f);
  }

  if (cycle_ >= ScanlineEndCycle) {
    // If cycle_ is greater than 340, it indicate go to the next scanline.
    // Find and index sprites that are on the next Scanline
    // This isn't where/when this indexing, actually copying in 2C02 is done
    // but (I think) it shouldn't hurt any games if this is done here
    scanlineSprites_.resize(0);
    int range = LONG_SPRITE() ? 16 : 8;
    for (std::size_t i = oamDataAddress_ / 4; i < 64; ++i) {
      auto diff = scanline_ - spriteMemory_[i * 4];
      if (0 <= diff && diff < range) {
        scanlineSprites_.push_back(i);
        if (scanlineSprites_.size() >= 8) break;
      }
    }

    ++scanline_;
    cycle_ = 0;
  }

  if (cycle_ == 256) {
    mainBus_.mapper()->Hsync(scanline_);
  }

  if (scanline_ >= VisibleScanlines) {
    // If scanline is greater than 240, it indicates the frame has been done.
    pipelineState_ = PostRender;
  }
}

void PPU::postRender() {
  if (cycle_ == 256) {
    mainBus_.mapper()->Hsync(scanline_);
  }

  if (cycle_ < ScanlineEndCycle) {
    // If cycle_ is less than 340, wait here.
    return;
  }

  ++scanline_;
  cycle_ = 0;
  pipelineState_ = VerticalBlank;

  for (int x = 0; x < pictureBuffer_.size(); ++x) {
    for (int y = 0; y < pictureBuffer_[0].size(); ++y) {
      screen_.setPixel(x, y, pictureBuffer_[x][y]);
    }
  }

    // Should technically be done at first dot of VBlank, but this is close
    // enough
#ifdef PPUSTATUS_IN_BYTE
  SET_BIT(ppu_status_, 0x80);
#else   // PPUSTATUS_IN_BYTE
  vblank_ = true;
#endif  // PPUSTATUS_IN_BYTE
  if (GEN_INTERRUPT()) {
    mainBus_.cpu()->TryNMI();
  }
}

void PPU::renderInScanline() {
#define READ_PIXEL(clr, addr, offset) \
  clr = ((read(addr) >> offset) & 1) | (((read(addr + 8) >> offset) & 1) << 1)

  Byte bgColor = 0, sprColor = 0;
  bool bgOpaque = false, sprOpaque = true;
  bool spriteForeground = false;

  int x = cycle_ - 1;
  int y = scanline_;

  if (SHOW_BACKGROUND()) {
    auto x_fine = 7 - ((fineXScroll_ + x) % 8);
    if (SHOW_EDGE_BACKGROUND() || x >= 8) {
      // fetch tile
      Address addr = 0x2000 | (dataAddress_ & 0x0FFF);  // mask off fine y
      Address tile = static_cast<Address>(read(addr));
      // auto addr = 0x2000 + x / 8 + (y / 8) * (ScanlineVisibleDots / 8);

      // fetch pattern
      // Each pattern occupies 16 bytes, so multiply by 16
      // Add fine y /* dataAddress_  y % 8*/
      // set whether the pattern is in the high or low page
      addr = (tile << 4) | ((dataAddress_ >> 12) & 0x7);
      if (HIGH_BG_PAGE()) addr |= 1 << 12;
      // Get the corresponding bit determined by x_fine from the right
      READ_PIXEL(bgColor, addr, x_fine);

      // flag used to calculate final pixel with the sprite pixel
      bgOpaque = bgColor;

      // fetch attribute and calculate higher two bits of palette
      // Attribute table start address is 0x23c0.
      addr = 0x23C0 | (dataAddress_ & 0x0C00) | ((dataAddress_ >> 4) & 0x38) |
             ((dataAddress_ >> 2) & 0x07);
      auto attribute = read(addr);
      int shift = ((dataAddress_ >> 4) & 4) | (dataAddress_ & 2);
      // Extract and set the upper two bits for the color
      bgColor |= ((attribute >> shift) & 0x3) << 2;
    }
    // Increment/wrap coarse X
    if (!x_fine) {
      if (TEST_BITS(dataAddress_, 0x001F)) {  // if coarse X == 31
        CLR_BIT(dataAddress_, 0x001F);        // coarse X = 0
        dataAddress_ ^= 0x0400;               // switch horizontal nametable
      } else {
        ++dataAddress_;  // increment coarse X
      }
    }
  }

  if (SHOW_SPRITES() && (SHOW_EDGE_SPRITES() || x >= 8)) {
    for (auto i : scanlineSprites_) {
      Byte *sprPtr = spriteMemory_.data() + (i * 4);
      int spr_x = x - sprPtr[3];
      if (0 > spr_x || spr_x >= 8) continue;

      int spr_y = y - sprPtr[0] - 1;
      int length = LONG_SPRITE() ? 16 : 8;
      int x_shift = 7 - (spr_x % 8), y_offset = spr_y % length;
      Byte tile = sprPtr[1], attribute = sprPtr[2];

      // If flipping horizontally
      if (TEST_BITS(attribute, 0x40)) x_shift ^= 7;
      // If flipping vertically
      if (TEST_BITS(attribute, 0x80)) y_offset ^= (length - 1);

      Address addr = 0;
      if (LONG_SPRITE()) {
        // 8x16 sprites

        // bit-3 is one if it is the bottom tile of the sprite, multiply by
        // two to get the next pattern
        y_offset = (y_offset & 7) | ((y_offset & 8) << 1);
        addr = (tile >> 1) * 32 + y_offset;
        addr |= (tile & 1) << 12;  // Bank 0x1000 if bit-0 is high
      } else {
        addr = tile * 16 + y_offset;
        if (HIGH_SPR_PAGE()) addr += 0x1000;
      }

      READ_PIXEL(sprColor, addr, x_shift);

      if ((sprOpaque = sprColor) != 0) {
        // Select sprite palette
        // bits 2-3
        sprColor |= 0x10 | ((attribute & 0x3) << 2);
        spriteForeground = !(attribute & 0x20);

        // Sprite-0 hit detection
        if (SHOW_BACKGROUND() && i == 0 && sprOpaque && bgOpaque) {
#ifdef PPUSTATUS_IN_BYTE
          SET_BIT(ppu_status_, 0x40);
#else   // PPUSTATUS_IN_BYTE
          sprZeroHit_ = true;
#endif  // PPUSTATUS_IN_BYTE
        }

        break;  // Exit the loop now since we've found the highest priority
                // sprite
      }
    }
  }

  Byte paletteAddr = bgColor;
  if (sprOpaque && (!bgOpaque || spriteForeground)) {
    paletteAddr = sprColor;
  } else if (!bgOpaque && !sprOpaque) {
    paletteAddr = 0;
  }
#undef READ_PIXEL

  pictureBuffer_[x][y] = bus_.readPalette(paletteAddr);
}

void PPU::doDMA(const Byte *page_ptr) {
  std::memcpy(spriteMemory_.data() + oamDataAddress_, page_ptr,
              256 - oamDataAddress_);
  if (oamDataAddress_) {
    std::memcpy(spriteMemory_.data(), page_ptr + (256 - oamDataAddress_),
                oamDataAddress_);
  }

  ///* TODO: enough with houdini */
  // page_ptr -= 256;
  //// Odd address in $2003
  // if (TEST_BITS(oamDataAddress_, 0x4)) {
  //  for (size_t oam_loc = 4; oam_loc < 8; oam_loc++)
  //    spriteMemory_[oam_loc] = *page_ptr++;
  //  page_ptr += 248;
  //  for (size_t oam_loc = 0; oam_loc < 4; oam_loc++)
  //    spriteMemory_[oam_loc] = *page_ptr++;
  //} else {
  //  // Even address in $2003
  //  for (size_t oam_loc = 0; oam_loc < 8; oam_loc++)
  //    spriteMemory_[oam_loc] = *page_ptr++;
  //}
}

// Write to 0x2000 in cpu memory
void PPU::control(Byte ctrl) {
#ifdef PPUCONTROL_IN_BYTE
  ppu_control_ = ctrl;
#else   // PPUCONTROL_IN_BYTE
  generateInterrupt_ = ctrl & 0x80;
  longSprites_ = ctrl & 0x20;
  bgPage_ = static_cast<CharacterPage>(!!(ctrl & 0x10));
  sprPage_ = static_cast<CharacterPage>(!!(ctrl & 0x8));
  // baseNameTable_ = (ctrl & 0x3) * 0x400 + 0x2000;
#endif  // PPUCONTROL_IN_BYTE

  dataAddrIncrement_ = (ctrl & 0x4) ? 0x20 : 1;
  // Set the nametable in the temp address, this will be reflected in the data
  // address during rendering
  CLR_BIT(tempAddress_, 0xc00);               // Unset
  SET_BIT(tempAddress_, (ctrl & 0x3) << 10);  // Set according to ctrl bits
}

// Write to 0x2001 in cpu memory
void PPU::setMask(Byte mask) {
#ifdef PPUMASK_IN_BYTE
  ppu_mask_ = mask;
#else   // PPUMASK_IN_BYTE
  greyscaleMode_ = mask & 0x1;
  showEdgeBackground_ = mask & 0x2;
  showEdgeSprites_ = mask & 0x4;
  showBackground_ = mask & 0x8;
  showSprites_ = mask & 0x10;
#endif  // PPUMASK_IN_BYTE
}

// Read to 0x2002 in cpu memory
Byte PPU::getStatus() {
  Byte status =
#ifdef PPUSTATUS_IN_BYTE
      ppu_status_ | 0x10;
#else   // PPUSTATUS_IN_BYTE
      sprZeroHit_ << 6 | vblank_ << 7
      //| ignoreVRAMWrite << 4
      ;
#endif  // PPUSTATUS_IN_BYTE
  firstWrite_ = true;
  return status;
}

// Write to 0x2003 in cpu memory
void PPU::setOAMAddress(Byte addr) { oamDataAddress_ = addr; }

// Write to 0x2004 in cpu memory
void PPU::setOAMData(Byte value) { spriteMemory_[oamDataAddress_++] = value; }
// Read to 0x2004 in cpu memory
Byte PPU::getOAMData() { return spriteMemory_[oamDataAddress_]; }

// Write to 0x2005 in cpu memory
void PPU::setScroll(Byte scroll) {
  if (firstWrite_) {
    fineXScroll_ = scroll & 0x7;
    scroll >>= 3;
    SWAP_BIT(tempAddress_, scroll, 0x1f);
  } else {
    Address addr = scroll;
    addr = ((addr << 12) & 0x7000) | ((addr << 2) & 0x3e0);
    SWAP_BIT(tempAddress_, addr, 0x73e0);
  }
  firstWrite_ = !firstWrite_;
}

// Write to 0x2006 in cpu memory
void PPU::setDataAddress(Byte val) {
  // dataAddress_ = ((dataAddress_ << 8) & 0xff00) | addr;
  Address addr = val;
  if (firstWrite_) {
    addr <<= 8;
    addr &= 0x3f00;
    SWAP_BIT(tempAddress_, addr, 0xff00);  // Unset the upper byte
  } else {
    SWAP_BIT(tempAddress_, addr, 0xff);  // Unset the lower byte;
    dataAddress_ = tempAddress_;
  }
  firstWrite_ = !firstWrite_;
}

// Read to 0x2007 in cpu memory
Byte PPU::getData() {
  Byte data = read(dataAddress_);
  dataAddress_ += dataAddrIncrement_;

  // Reads are delayed by one byte/read when address is in this range
  if (dataAddress_ < 0x3f00) {
    // Return from the data buffer and store the current value in the buffer
    std::swap(data, dataBuffer_);
  }

  return data;
}
// Write to 0x2007 in cpu memory
void PPU::setData(Byte data) {
  bus_.write(dataAddress_, data);
  dataAddress_ += dataAddrIncrement_;
}

inline Byte PPU::read(Address addr) { return bus_.read(addr); }

void PPU::DebugDump() {
  LOG(INFO) << "PPU: " << frameIndex_ << " sc:" << scanline_ << "," << cycle_
            << " state:" << pipelineState_ << std::hex
#ifdef PPUSTATUS_IN_BYTE
            << " status:" << +ppu_status_
#endif  // PPUSTATUS_IN_BYTE
#ifdef PPUMASK_IN_BYTE
            << " mask:" << +ppu_mask_
#endif  // PPUMASK_IN_BYTE
#ifdef PPUCONTROL_IN_BYTE
            << " control:" << +ppu_control_
#endif  // PPUCONTROL_IN_BYTE
            << " @" << tempAddress_ << " dataAddr: $" << dataAddress_
            << " 1stW: " << std::boolalpha << firstWrite_
            << " xScl:" << +fineXScroll_ << " addrBuf:" << +dataBuffer_
            << " oamAddr:" << +oamDataAddress_;
}

}  // namespace hn
